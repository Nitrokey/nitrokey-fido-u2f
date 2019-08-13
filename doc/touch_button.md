## Touch button state machine diagram

![Touch button state machine][touch-button]

### States description
Excerpt from `gpio.h`:
```
typedef enum {
	BST_INITIALIZING,			// wait for the charge to settle down
	BST_INITIALIZING_READY_TO_CLEAR,	// ready for clearing
	BST_READY_TO_USE,			// META state (never used), to ease testing,
						        // if button is ready (e.g. >READY) or not (<READY)
	BST_UNPRESSED,				// ready to use
	BST_PRESSED_RECENTLY,		// touch registration is started
	BST_PRESSED_REGISTERED,		// touch registered, normal press period
	BST_PRESSED_REGISTERED_EXT, // touch registered, extended press period
	BST_PRESSED_CONSUMED,		// touch registered and consumed, but button still not released

	BST_MAX_NUM
} BUTTON_STATE_T;
```

[touch-button]: images/touch-button-5.svg "Touch button state machine"
