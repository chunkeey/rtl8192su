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
#ifndef __INC_HAL8192SELED_H
#define __INC_HAL8192SELED_H



typedef	enum _LED_STATE_8190{
	LED_UNKNOWN = 0,
	LED_ON = 1,
	LED_OFF = 2,
	LED_BLINK_NORMAL = 3,
	LED_BLINK_SLOWLY = 4,
	LED_POWER_ON_BLINK = 5,
	LED_SCAN_BLINK = 6, 
	LED_NO_LINK_BLINK = 7, 
	LED_BLINK_StartToBlink = 8, 
	LED_BLINK_TXRX = 9,
	LED_BLINK_RUNTOP = 10, 
	LED_BLINK_CAMEO = 11,
}LED_STATE_8190;

typedef enum _LED_PIN_8190{
	LED_PIN_GPIO0,
	LED_PIN_LED0,
	LED_PIN_LED1
}LED_PIN_8190;

typedef struct _LED_8190{
	void *			dev;

	LED_PIN_8190	LedPin;	

	LED_STATE_8190	CurrLedState; 
	bool				bLedOn; 

	bool				bLedBlinkInProgress; 

	bool				bLedSlowBlinkInProgress;
	u32				BlinkTimes; 
	LED_STATE_8190	BlinkingLedState; 

	struct timer_list 	BlinkTimer; 
} LED_8190, *PLED_8190;




typedef	enum _LED_STRATEGY_8190{
	SW_LED_MODE0, 
	SW_LED_MODE1, 
	SW_LED_MODE2, 
	SW_LED_MODE3, 
	SW_LED_MODE4, 
	SW_LED_MODE5, 
	SW_LED_MODE6, 
	SW_LED_MODE7, 
	SW_LED_MODE8,
	SW_LED_MODE9, 
	SW_LED_MODE10,
	HW_LED, 
}LED_STRATEGY_8190, *PLED_STRATEGY_8190;


void SwLedOn(struct net_device *dev , PLED_8190 pLed);
void SwLedOff(struct net_device *dev, PLED_8190 pLed);
void InitSwLeds(struct net_device *dev);
void DeInitSwLeds(struct net_device *dev);
void LedControl8192SE(struct net_device *dev, LED_CTL_MODE LedAction);

#endif	/*__INC_HAL8190PCILED_H*/
