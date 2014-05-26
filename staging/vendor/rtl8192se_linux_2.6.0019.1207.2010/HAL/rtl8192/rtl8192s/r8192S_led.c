/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
#include "../rtl_core.h"
#ifdef RTL8192SE
#define LED_BLINK_NORMAL_INTERVAL	100
#define LED_BLINK_SLOWLY_INTERVAL		200

#define LED_BLINK_NORMAL_INTERVAL_NETTRONIX  100
#define LED_BLINK_SLOWLY_INTERVAL_NETTRONIX  2000

#define LED_BLINK_SLOWLY_INTERVAL_PORNET 1000
#define LED_BLINK_NORMAL_INTERVAL_PORNET 100


#define LED_CM2_BLINK_ON_INTERVAL			250
#define LED_CM2_BLINK_OFF_INTERVAL		4750

#define LED_CM8_BLINK_OFF_INTERVAL		3750	

#define LED_RunTop_BLINK_INTERVAL 			300

#define LED_CM3_BLINK_INTERVAL			1500



static void BlinkTimerCallback(unsigned long data);


void InitLed8190Pci(struct net_device *dev, PLED_8190 	pLed,LED_PIN_8190 LedPin)
{
	pLed->dev = dev;
	pLed->LedPin = LedPin;

	pLed->CurrLedState = LED_OFF;
	pLed->bLedOn = false;
	pLed->bLedBlinkInProgress = false;
	pLed->BlinkTimes = 0;
	pLed->BlinkingLedState = LED_OFF;

	setup_timer(&pLed->BlinkTimer,
		    BlinkTimerCallback,
		    (unsigned long) pLed);
}

void DeInitLed8190Pci(PLED_8190 pLed)
{
	del_timer_sync(&(pLed->BlinkTimer));
	pLed->bLedBlinkInProgress = false;
}

void SwLedOn(	struct net_device *dev , PLED_8190 pLed)
{
	u8	LedCfg;

	LedCfg = read_nic_byte(dev, LEDCFG);
	
	switch(pLed->LedPin)
	{
	case LED_PIN_GPIO0:
		break;

	case LED_PIN_LED0:
		write_nic_byte(dev, LEDCFG, LedCfg&0xf0); 
		break;

	case LED_PIN_LED1:
		write_nic_byte(dev, LEDCFG, LedCfg&0x0f); 
		break;

	default:
		break;
	}

	pLed->bLedOn = true;
}

void SwLedOff(struct net_device *dev, PLED_8190 pLed)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8	LedCfg;
	
	LedCfg = read_nic_byte(dev, LEDCFG);
	
	switch(pLed->LedPin)
	{
	case LED_PIN_GPIO0:
		break;

	case LED_PIN_LED0:
		LedCfg &= 0xf0; 

		if(priv->bLedOpenDrain == true) 
			write_nic_byte(dev, LEDCFG, (LedCfg|BIT1));
		else
			write_nic_byte(dev, LEDCFG, (LedCfg|BIT3));
		break;

	case LED_PIN_LED1:
		LedCfg &= 0x0f; 
		write_nic_byte(dev, LEDCFG, (LedCfg|BIT7));
		break;

	default:
		break;
	}

	pLed->bLedOn = false;
}


void InitSwLeds(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	InitLed8190Pci(
		dev,
		&(priv->SwLed0), 
		LED_PIN_LED0);

	InitLed8190Pci(
		dev,
		&(priv->SwLed1), 
		LED_PIN_LED1);
}


void DeInitSwLeds(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	DeInitLed8190Pci( &(priv->SwLed0) );
	DeInitLed8190Pci( &(priv->SwLed1) );
}

void HwLedBlink(struct net_device *dev, PLED_8190 pLed)
{

	
	switch(pLed->LedPin)
	{
	case LED_PIN_GPIO0:
		break;

	case LED_PIN_LED0:
		break;

	case LED_PIN_LED1:
		break;

	default:
		break;
	}

	pLed->bLedOn = true;
}



void SwLedBlink(PLED_8190 pLed)
{
	struct net_device *dev = (struct net_device *)pLed->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	bool bStopBlinking = false;
	
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}	
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}

	pLed->BlinkTimes--;
	switch(pLed->CurrLedState)
	{
	case LED_BLINK_NORMAL: 
	case LED_BLINK_TXRX:
	case LED_BLINK_RUNTOP:
		if(pLed->BlinkTimes == 0)
		{
			bStopBlinking = true;
		}
		break;

	case LED_SCAN_BLINK:
		if( (priv->rtllib->state == RTLLIB_LINKED) &&  
			(!rtllib_act_scanning(priv->rtllib,true))&& 
			(pLed->BlinkTimes % 2 == 0)) 
		{
			bStopBlinking = true;
		}
		break;

	case LED_NO_LINK_BLINK:
	case LED_BLINK_StartToBlink:	
		if( (priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->iw_mode == IW_MODE_INFRA)) 
		{
			bStopBlinking = true;
		}
		else if((priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->iw_mode == IW_MODE_ADHOC))
		{
			bStopBlinking = true;
		}
		else if(pLed->BlinkTimes == 0)
		{
			bStopBlinking = true;
		}
		break;
		
	case LED_BLINK_CAMEO:
		if((priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->iw_mode == IW_MODE_INFRA)) 
		{
			bStopBlinking = true;
		}
		else if((priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->iw_mode == IW_MODE_ADHOC) )
		{
			bStopBlinking = true;
		}
		break;
		
	default:
		bStopBlinking = true;
		break;
	}

	if(bStopBlinking)
	{
		if( priv->rtllib->eRFPowerState != eRfOn )
		{
			SwLedOff(dev, pLed);
		}
		else if(pLed->CurrLedState == LED_BLINK_TXRX)
		{
			SwLedOff(dev, pLed);
		}
		else if(pLed->CurrLedState == LED_BLINK_RUNTOP)
		{
			SwLedOff(dev, pLed);
		}
		else if( (priv->rtllib->state == RTLLIB_LINKED) && (pLed->bLedOn == false))
		{
			SwLedOn(dev, pLed);
		}
		else if( (priv->rtllib->state != RTLLIB_LINKED) &&  pLed->bLedOn == true)
		{
			SwLedOff(dev, pLed);
		}

		pLed->BlinkTimes = 0;
		pLed->bLedBlinkInProgress = false;	
	}
	else
	{
		if( pLed->BlinkingLedState == LED_ON ) 
			pLed->BlinkingLedState = LED_OFF;
		else 
			pLed->BlinkingLedState = LED_ON;

		switch( pLed->CurrLedState )
		{
		case LED_BLINK_NORMAL:
		case LED_BLINK_TXRX:
		case LED_BLINK_StartToBlink:
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			break;

		case LED_BLINK_SLOWLY:
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
			break;

		case LED_SCAN_BLINK:
		case LED_NO_LINK_BLINK:
			if( pLed->bLedOn )
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
			else
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_OFF_INTERVAL));
			break;
			
		case LED_BLINK_RUNTOP:
			mod_timer(&(pLed->BlinkTimer),jiffies + MSECS(LED_RunTop_BLINK_INTERVAL));
			break;
			
		case LED_BLINK_CAMEO:
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_PORNET));
			break;
			
		default:
			RT_TRACE(COMP_ERR, "SwLedCm2Blink(): unexpected state!\n");
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
			break;
		}		
	}
}

void SwLedBlink5(PLED_8190 pLed)
{
	struct net_device *dev = (struct net_device *)pLed->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	bool bStopBlinking = false;

	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}	
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}

	switch(pLed->CurrLedState)
	{
		case LED_OFF:
			SwLedOff(dev, pLed);
			break;

		case LED_BLINK_SLOWLY:
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_NETTRONIX));
			break;

		case LED_BLINK_NORMAL:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->bLedSlowBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_SLOWLY;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_NETTRONIX));
				}
				pLed->BlinkTimes = 0;
				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 

					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL_NETTRONIX));
				}
			}
			break;
			
		default:
			break;	
		}

}


void SwLedBlink6(PLED_8190 pLed)
{
	struct net_device *dev = (struct net_device *)pLed->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
	bool bStopBlinking = false;

	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}	
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}
	
	switch(pLed->CurrLedState)
	{
		case LED_OFF:
			SwLedOff(dev, pLed);
			break;

		case LED_BLINK_SLOWLY:
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_PORNET));
			break;

		case LED_BLINK_NORMAL:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->bLedSlowBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_SLOWLY;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_PORNET));
				}
				pLed->BlinkTimes = 0;
				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 

					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL_PORNET));
				}
			}
			break;
			
		default:
			break;	
		}
	
}

void SwLedBlink7(	PLED_8190 pLed)
{
	struct net_device *dev = (struct net_device *)pLed->dev;

	SwLedOn(dev, pLed);
	RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);	
}

void BlinkTimerCallback(unsigned long data)
{	
	PLED_8190 	pLed = (PLED_8190)data;
	struct net_device *dev = (struct net_device *)pLed->dev;
	struct r8192_priv *priv = rtllib_priv(dev);
		
	switch(priv->LedStrategy)
	{
	case SW_LED_MODE1:
		break;
	case SW_LED_MODE2:
		break;
	case SW_LED_MODE3:
		break;
	case SW_LED_MODE5:
		break;
	case SW_LED_MODE6:
		break;

	case SW_LED_MODE7:
		SwLedBlink7(pLed);
		break;

	default:
		break;
	}	
}

void SwLedControlMode0(struct net_device *dev,LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);
	PLED_8190 pLed1 = &(priv->SwLed1);

	switch(LedAction)
	{
	case LED_CTL_TX:
	case LED_CTL_RX:	
		break;

	case LED_CTL_LINK:
		pLed0->CurrLedState = LED_ON;
		SwLedOn(dev, pLed0);

		pLed1->CurrLedState = LED_BLINK_NORMAL;
		HwLedBlink(dev, pLed1);
		break;

	case LED_CTL_POWER_ON:
		pLed0->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed0);

		pLed1->CurrLedState = LED_BLINK_NORMAL;
		HwLedBlink(dev, pLed1);

		break;

	case LED_CTL_POWER_OFF:
		pLed0->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed0);

		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);
		break;

	case LED_CTL_SITE_SURVEY:
		break;

	case LED_CTL_NO_LINK:
		pLed0->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed0);

		pLed1->CurrLedState = LED_BLINK_NORMAL;
		HwLedBlink(dev, pLed1);
		break;	

	default:
		break;
	}

	RT_TRACE(COMP_LED, "Led0 %d Led1 %d\n", pLed0->CurrLedState, pLed1->CurrLedState);
}


void SwLedControlMode1(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190	pLed = &(priv->SwLed1);
	
	switch(LedAction)
	{
	case LED_CTL_TX:
	case LED_CTL_RX:
		if( pLed->bLedBlinkInProgress == false )
		{
			pLed->bLedBlinkInProgress = true;

			pLed->CurrLedState = LED_BLINK_NORMAL;
			pLed->BlinkTimes = 2;

			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
		}
		break;

	case LED_CTL_SITE_SURVEY:
		if( pLed->bLedBlinkInProgress == false )
		{
			pLed->bLedBlinkInProgress = true;

			if(priv->rtllib->state == RTLLIB_LINKED)
			{
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 4;
			}
			else
			{
				pLed->CurrLedState = LED_NO_LINK_BLINK;
				pLed->BlinkTimes = 24;
			}

			if( pLed->bLedOn )
			{
				pLed->BlinkingLedState = LED_OFF; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
			}
			else
			{
				pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_OFF_INTERVAL));
			}
		}
		else
		{
			if(pLed->CurrLedState != LED_NO_LINK_BLINK)
			{
				if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_SCAN_BLINK;
				}
				else
				{
					pLed->CurrLedState = LED_NO_LINK_BLINK;
				}
			}
		}
		break;

	case LED_CTL_NO_LINK:
		if( pLed->bLedBlinkInProgress == false )
		{
			pLed->bLedBlinkInProgress = true;

			pLed->CurrLedState = LED_NO_LINK_BLINK;
			pLed->BlinkTimes = 24;

			if( pLed->bLedOn )
			{
				pLed->BlinkingLedState = LED_OFF; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
			}
			else
			{
				pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_OFF_INTERVAL));
			}
		}
		else
		{
			pLed->CurrLedState = LED_NO_LINK_BLINK;
		}
		break;

	case LED_CTL_LINK:
		pLed->CurrLedState = LED_ON;
		if( pLed->bLedBlinkInProgress == false )
		{
			SwLedOn(dev, pLed);
		}
		break;

	case LED_CTL_POWER_OFF:
		pLed->CurrLedState = LED_OFF;
		if(pLed->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed->BlinkTimer));
			pLed->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed);
		break;

	default:
		break;
	}
	
	RT_TRACE(COMP_LED, "Led %d \n", pLed->CurrLedState);
}

void SwLedControlMode2(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);
	PLED_8190 pLed1 = &(priv->SwLed1);
	
	switch(LedAction)
	{
	case LED_CTL_POWER_ON:
		pLed0->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed0);
		
		pLed1->CurrLedState = LED_BLINK_CAMEO;
		if( pLed1->bLedBlinkInProgress == false )
		{
			pLed1->bLedBlinkInProgress = true;

			pLed1->BlinkTimes = 6;
		
			if( pLed1->bLedOn )
				pLed1->BlinkingLedState = LED_OFF; 
			else
				pLed1->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed1->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_PORNET));
		}
		break;

	case LED_CTL_TX:
	case LED_CTL_RX:
		if( pLed0->bLedBlinkInProgress == false )
		{
			pLed0->bLedBlinkInProgress = true;

			pLed0->CurrLedState = LED_BLINK_TXRX;
			pLed0->BlinkTimes = 2;

			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
			
			mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
		}
		break;

	case LED_CTL_NO_LINK:
		pLed1->CurrLedState = LED_BLINK_CAMEO;
		if( pLed1->bLedBlinkInProgress == false )
		{
			pLed1->bLedBlinkInProgress = true;
			
			pLed1->BlinkTimes = 6;

			if( pLed1->bLedOn )
				pLed1->BlinkingLedState = LED_OFF; 
			else
				pLed1->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed1->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_PORNET));
		}
		break;

	case LED_CTL_LINK:
		pLed1->CurrLedState = LED_ON;
		if( pLed1->bLedBlinkInProgress == false )
		{
			SwLedOn(dev, pLed1);
		}
		break;

	case LED_CTL_POWER_OFF:
		pLed0->CurrLedState = LED_OFF;
		pLed1->CurrLedState = LED_OFF;
		if(pLed0->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedBlinkInProgress = false;
		}
		if(pLed1->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed1->BlinkTimer));
			pLed1->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed0);
		SwLedOff(dev, pLed1);
		break;

	default:
		break;
	}
	
	RT_TRACE(COMP_LED, "Led0 %d, Led1 %d \n", pLed0->CurrLedState, pLed1->CurrLedState);
}



void SwLedControlMode3(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);
	PLED_8190 pLed1 = &(priv->SwLed1);
	
	switch(LedAction)
	{
	case LED_CTL_POWER_ON:
		pLed0->CurrLedState = LED_ON;
		SwLedOn(dev, pLed0);
		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);
		break;
		
	case LED_CTL_TX:
	case LED_CTL_RX:
		if( pLed1->bLedBlinkInProgress == false )
		{
			pLed1->bLedBlinkInProgress = true;

			pLed1->CurrLedState = LED_BLINK_RUNTOP;
			pLed1->BlinkTimes = 2;
		
			if( pLed1->bLedOn )
				pLed1->BlinkingLedState = LED_OFF; 
			else
				pLed1->BlinkingLedState = LED_ON; 
			
			mod_timer(&(pLed1->BlinkTimer), jiffies + MSECS(LED_RunTop_BLINK_INTERVAL));
		}
		break;

	case LED_CTL_POWER_OFF:
		pLed0->CurrLedState = LED_OFF;
		pLed1->CurrLedState = LED_OFF;
		if(pLed0->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedBlinkInProgress = false;
		}
		if(pLed1->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed1->BlinkTimer));
			pLed1->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed0);
		SwLedOff(dev, pLed1);
		break;

	default:
		break;
	}
	
	RT_TRACE(COMP_LED, "Led0 %d, Led1 %d \n", pLed0->CurrLedState, pLed1->CurrLedState);
}


void SwLedControlMode4(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);
	PLED_8190 pLed1 = &(priv->SwLed1);
	
	switch(LedAction)
	{
	case LED_CTL_POWER_ON:
		pLed1->CurrLedState = LED_ON;
		SwLedOn(dev, pLed1);
		pLed0->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed0);
		break;
		
	case LED_CTL_TX:
	case LED_CTL_RX:
		if( pLed0->bLedBlinkInProgress == false )
		{
			pLed0->bLedBlinkInProgress = true;

			pLed0->CurrLedState = LED_BLINK_RUNTOP;
			pLed0->BlinkTimes = 2;
		
			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
			
			mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_RunTop_BLINK_INTERVAL));
		}
		break;

	case LED_CTL_POWER_OFF:
		pLed0->CurrLedState = LED_OFF;
		pLed1->CurrLedState = LED_OFF;
		if(pLed0->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedBlinkInProgress = false;
		}
		if(pLed1->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed1->BlinkTimer));
			pLed1->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed0);
		SwLedOff(dev, pLed1);
		break;

	default:
		break;
	}
	
	RT_TRACE(COMP_LED, "Led0 %d, Led1 %d \n", pLed0->CurrLedState, pLed1->CurrLedState);
}

void SwLedControlMode5(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);
	PLED_8190 pLed1 = &(priv->SwLed1);
	switch(LedAction)
	{
	case LED_CTL_POWER_ON:
	case LED_CTL_START_TO_LINK:
	case LED_CTL_NO_LINK:
		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);


		if( pLed0->bLedSlowBlinkInProgress == false )
		{
			pLed0->bLedSlowBlinkInProgress = true;
			pLed0->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_NETTRONIX));
		}
		
		break;
	
	case LED_CTL_TX:
	case LED_CTL_RX:	
		pLed1->CurrLedState = LED_ON;
		SwLedOn(dev, pLed1);

		if( pLed0->bLedBlinkInProgress == false )
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedSlowBlinkInProgress = false;
			pLed0->bLedBlinkInProgress = true;
			pLed0->CurrLedState = LED_BLINK_NORMAL;
			pLed0->BlinkTimes = 2;

			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL_NETTRONIX));
		}		
		break;

	case LED_CTL_LINK:
		pLed1->CurrLedState = LED_ON;
		SwLedOn(dev, pLed1);

		if( pLed0->bLedSlowBlinkInProgress == false )
		{
			pLed0->bLedSlowBlinkInProgress = true;
			pLed0->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_NETTRONIX));
		}
		break;


	case LED_CTL_POWER_OFF:
		pLed0->CurrLedState = LED_OFF;
		pLed1->CurrLedState = LED_OFF;
		if( pLed0->bLedSlowBlinkInProgress == true )
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedSlowBlinkInProgress = false;
		}
		if(pLed0->bLedBlinkInProgress == true)
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed0);
		SwLedOff(dev, pLed1);
		break;

	default:
		break;
		}

	
}

void SwLedControlMode6(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);
	PLED_8190 pLed1 = &(priv->SwLed1);
	

	switch(LedAction)
	{
	case LED_CTL_POWER_ON:
	case LED_CTL_START_TO_LINK:
	case LED_CTL_NO_LINK:
	case LED_CTL_LINK:
	case LED_CTL_SITE_SURVEY:
		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);
		
		if( pLed0->bLedSlowBlinkInProgress == false )
		{
			pLed0->bLedSlowBlinkInProgress = true;
			pLed0->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL_PORNET));
		}
		break;

	case LED_CTL_TX:
	case LED_CTL_RX:
		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);
		if( pLed0->bLedBlinkInProgress == false )
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedSlowBlinkInProgress = false;
			pLed0->bLedBlinkInProgress = true;
			pLed0->CurrLedState = LED_BLINK_NORMAL;
			pLed0->BlinkTimes = 2;
			if( pLed0->bLedOn )
				pLed0->BlinkingLedState = LED_OFF; 
			else
				pLed0->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed0->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL_PORNET));
		}		
		break;

	case LED_CTL_POWER_OFF:
		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);
		
		pLed0->CurrLedState = LED_OFF;
		if( pLed0->bLedSlowBlinkInProgress == true )
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedSlowBlinkInProgress = false;
		}
		if(pLed0->bLedBlinkInProgress == true)
		{
			del_timer_sync(&(pLed0->BlinkTimer));
			pLed0->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed0);
		break;

	default:
		break;
		
		}
}


void SwLedControlMode7(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);	

	switch(LedAction)
	{
		case LED_CTL_POWER_ON:
		case LED_CTL_LINK:
		case LED_CTL_NO_LINK:			
			SwLedOn(dev, pLed0);
			break;

		case LED_CTL_POWER_OFF:
				SwLedOff(dev, pLed0);
			break;

		default:
			break;
			
		}
}

void
SwLedControlMode8(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed = &(priv->SwLed0);	

	switch(LedAction)
	{
		case LED_CTL_TX:
		case LED_CTL_RX:
			if( pLed->bLedBlinkInProgress == false && priv->rtllib->state == RTLLIB_LINKED)
			{
				pLed->bLedBlinkInProgress = true;

				pLed->CurrLedState = LED_BLINK_NORMAL;
				pLed->BlinkTimes = 2;

				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			}
			break;

		case LED_CTL_SITE_SURVEY:
			if( pLed->bLedBlinkInProgress == false )
			{
				pLed->bLedBlinkInProgress = true;
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 2;
			
				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM8_BLINK_OFF_INTERVAL));
				}				
			}
			else if(pLed->CurrLedState != LED_SCAN_BLINK)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 2;

				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM8_BLINK_OFF_INTERVAL));
				}				
			}			
			break;

		case LED_CTL_POWER_ON:
		case LED_CTL_NO_LINK:
			if( pLed->bLedBlinkInProgress == false )
			{
				pLed->bLedBlinkInProgress = true;

				pLed->CurrLedState = LED_NO_LINK_BLINK;
				pLed->BlinkTimes = 24;

				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM8_BLINK_OFF_INTERVAL));
				}
			}
			else if(pLed->CurrLedState != LED_SCAN_BLINK && pLed->CurrLedState != LED_NO_LINK_BLINK)
			{
				pLed->CurrLedState = LED_NO_LINK_BLINK;
				pLed->BlinkTimes = 24;

				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM2_BLINK_ON_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_CM8_BLINK_OFF_INTERVAL));
				}
			}
			break;

		case LED_CTL_LINK:
			pLed->CurrLedState = LED_ON;
			if(pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			SwLedOn(dev, pLed);
			break;

		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			if(pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			SwLedOff(dev, pLed);
			break;

		default:
			break;
		}
}

void
SwLedControlMode9(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed = &(priv->SwLed0);	
	
		switch(LedAction)
		{
		case LED_CTL_TX:
		case LED_CTL_RX:
			if( pLed->bLedBlinkInProgress == false )
			{
				pLed->bLedBlinkInProgress = true;
	
				pLed->CurrLedState = LED_BLINK_NORMAL;
				pLed->BlinkTimes = 2;
	
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			}
			break;
	
		case LED_CTL_SITE_SURVEY:
		 	if(priv->rtllib->LinkDetectInfo.bBusyTraffic && priv->rtllib->state == RTLLIB_LINKED)
			 	;			
			else 
			{
				if( pLed->bLedBlinkInProgress == true )			
					del_timer_sync(&(pLed->BlinkTimer));
				else		
					pLed->bLedBlinkInProgress = true;
	
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 24;
	
				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
				}
			}	
			break;
	
		case LED_CTL_POWER_ON:
		case LED_CTL_NO_LINK:
		case LED_CTL_LINK:
			if( pLed->bLedBlinkInProgress == false )
			{
				pLed->CurrLedState = LED_ON;		
				SwLedOn(dev, pLed);
			}
			break;
	
		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			if(pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			SwLedOff(dev, pLed);
			break;
	
		default:
			break;
		}
	
		RT_TRACE(COMP_LED, "LED9 CurrLedState %d\n", pLed->CurrLedState);
		
}

void
SwLedControlMode10(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);	

	switch(LedAction)
	{
		case LED_CTL_LINK:
			SwLedOn(dev, pLed0);
			break;
			
		case LED_CTL_POWER_ON:	
		case LED_CTL_NO_LINK:			
		case LED_CTL_POWER_OFF:
			SwLedOff(dev, pLed0);
			break;

		default:
			break;			
	}
}

void LedControl8192SE(struct net_device *dev, LED_CTL_MODE LedAction)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if(IS_NIC_DOWN(priv))
		return;

	if(priv->bdisable_nic == true) 
		return;
	
#ifdef TO_DO_LIST
	if(Adapter->bInHctTest)
		return;
#endif

	if(	priv->rtllib->eRFPowerState != eRfOn && 
		(LedAction == LED_CTL_TX || LedAction == LED_CTL_RX || 
		 LedAction == LED_CTL_SITE_SURVEY || 
		 LedAction == LED_CTL_LINK || 
		 LedAction == LED_CTL_NO_LINK||
		 LedAction == LED_CTL_START_TO_LINK ||
		 LedAction == LED_CTL_POWER_ON) )
	{
		return;
	}

	RT_TRACE(COMP_LED, "LedAction %d, \n", LedAction);

	switch(priv->LedStrategy)
	{
	case SW_LED_MODE0:
		break;
	case SW_LED_MODE1:
		break;
	case SW_LED_MODE2:
		break;
	case SW_LED_MODE3:
		break;
	case SW_LED_MODE4:
		break;
	case SW_LED_MODE5: 
		break;
		
	case SW_LED_MODE6: 
		break;

	case SW_LED_MODE7:
		SwLedControlMode7(dev, LedAction);
		break;
	case SW_LED_MODE8:
		SwLedControlMode8(dev, LedAction);
		break;

	case SW_LED_MODE9:
		SwLedControlMode9(dev, LedAction);
		break;
	case SW_LED_MODE10:		
		SwLedControlMode10(dev, LedAction);
		break;	

	default:
		break;
	}	
}

#ifdef TO_DO_LIST
#ifdef NDIS50_MINIPORT
void LedBlinkTimerStallCheck(struct net_device *dev)
{	
}


void DoLedTimerStallCheck(PLED_8190 pLed)
{	
}
#endif 
#endif
#endif
