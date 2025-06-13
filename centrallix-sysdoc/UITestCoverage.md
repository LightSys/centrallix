# UI Test coverage
## Date
June 2025
## Author
David Hopkins, Minsik Lee
## Synopsis
This document outlines the current coverage of UI tests and test result format.
Each test includes:
- The properties being tested
- Test descriptions, including conditions for pass/fail results
## Test Result Format
### Format Structure

```text
TEST [#] = [Test Name / Description]
    Test [specific check description] ... [PASS/FAIL]
    Test [specific check description] ... [PASS/FAIL]
([number passed]/[total number of checks]) [PASS/FAIL]

[Component] Test [PASS/FAIL]
```

### Example
```text
TEST 1 = Hover behavior test
  Test pointimage change ... PASS
  Test tristate change ... PASS
(2/2) PASS

TEST 2 = Click behavior test
  Test click event ... FAIL
(0/1) FAIL

Button Test FAIL
```

----------

## Button Test
### Properties Tested
- `type`: text, image, topimage, rightimage, leftimage, bottomimage
- `enabled`: yes, no
- `tristate`: yes, no
- `clickimage`
- `disabledimage`
- `pointimage`

### 1. Hover Behavior Test
- Verifies that the `pointimage` changes on hover
- Verifies that the `tristate` option updates the border style appropriately
### 2. Click Behavior Test
- Verifies that clicking the button updates the connected label’s value
#### Note
- The `textoverImgButton` component appears to be non-functional; additionally, other buttons stop responding when it is added to the ge.
- Buttons with `text` type display only the `clickimage` when clicked. Other button types display both the default image and `clickimage` when clicked.

----------

## ChildWindow Test
### Properties Tested

- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`, `.wntitlebar`
- **JavaScript Initialization:** `pg_isloaded`, `wn_topwin`
- **Mouse Interactivity:** Drag-and-drop, double-click, single-click
- **Component State:** Minimized, restored, closed, modal set to true, top level set to true, titlebar set to true 

### 1. Initialization Test

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Interaction and State Change Test

- Verifies that the window's title bar is present and interactive
- Confirms that the window can be repositioned by simulating a drag-and-drop action on the title bar
- Verifies that a double-click on the title bar correctly toggles the window's state between minimized and restored

### 3. Closing Behavior Test

- Verifies that the window is successfully removed from the view when its close button is clicked

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## DateTime Test
### Properties Tested

- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Mouse Interactivity:** Single-click, typing into textbox
- **Component State:** Different year, month, and day, clear date, set date to today, enter date into EditBox 

### 1. Initialization Test

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Datetime Widget Features 

- Verifies that the widget can be located and opened
- Verifies that the No Date Button can be clicked AND presents no date
- Verifies that the Today Button can be clicked and accurately represents the current date

### 3. Edit Box Direct Input Test

- Verifies that typing directly into the edit box works 
- Verifies that clearing the edit box works 

### 4. Calendar Navigation and Selection
- Verifies that when a random day is selected, it navigates the year, month, and selects the right date 

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.
- Test MAY fail. It is not 100% done yet. One point of failure is when a random day could not be chosen thus failing the test. 

----------

## CMPDECL (Component Declaration) Part 1 Test
### Properties Tested

- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Mouse Interactivity:** Single-click, typing into textbox
- **Component State:** Inserting various types of random characters (letters, numbers, symbols, etc.) and button clicking. Futhermore, this is part of component declaration widget testing which includes parameter widget testing and component widget testing. This test is made of a .cmp and .app file. 

### 1. Initialization Test

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Text Area Interaction 

- Verifies that text area is found 
- Verifies that text area is clickable 
- Verifies that initial text (the one that is already written) can be sent 
- Verifies that a large random text can be sent 

### 3. Button Interaction

- Retrieve that the text is "Hello David" 
- Attempted 5 clicks on the button 

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.
- This is just part 1 of the CMPDECL (Component Declaration) Test.

----------

## EditBox Test
### Properties Tested

- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Mouse Interactivity:** Single-click, typing into textbox, focused keyboard 
- **Component State:** Inserting text into textbox

### 1. Initialization Test

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Text Area Interaction 

- Verifies that text area is found 
- Verifies that text area is clickable 
- Verifies that initial text (the one that is already written) can be sent 
- Verifies that a large random text can be sent 

### 3. Button Interaction

- Retrieve that the text is "Hello David" 
- Attempted 5 clicks on the button 

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.
- This is just part 1 of the CMPDECL (Component Declaration) Test.

----------

## Checkbox Test
### Properties Tested
- `checked`: yes, no
- `readonly`: yes, no

### 1. Click Behavior Test
- Verifies that the img src changes to `checkbox_checked.gif` -> `checkbox_null.gif` -> `checkbox_unchecked.gif` in order.

#### Note
- Readonly checkbox is clickable and its img source gets updated,

----------

## HTML Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Mouse Interactivity:** Single-click, typing into textbox, focused keyboard 
- **Component State:** Inserting text into textbox, static HTML, source correcly loaded 

### 1. Initialization and Content Verification Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript
- Verifies the HTML content length 
- Verifies that the page title and the first header

### 2. Interactive Element Test
- Verifies sending text into an inut field 
- Verifies that the response message received is according
- Verifies clearing the text field using backspaces
- Verifies that the text field is empty after clearing

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.
- HTML is static, and the HTML could be found under the UI test file (index.html)


----------

## ImageButton Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Mouse Interactivity:** Single-click, double-click, focused keyboard 
- **Component State:** two-state button, tristate button, enabled/disabled button. Hovering over the imagebutton 

### 1. Initialization and Content Verification Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Interactive Element Test
- Verifies TwoStateBtn can be clicked
- Verifies ThreeStatebtn can be clicked and hovered 
- Verifes EnableBtn click and verification
- Verifies clicking the now-enabled 'DisabledBtn'
- Verifies DisableBtn click can be disabled
- Verifies final check confirms button is disabled

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## Pane Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Making sure 5 panes are there

### 1. Initialization and Pane Verification Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript
- Verifies that 5 panes are there: lowered, raised, bordered, transparent, and flat

#### Note

- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## Clock Test
### Properties Tested
- `event`: MouseOver, MouseMove, MouseDown, MouseUp
### 1. Hover Behavior Test
- Verifies that the connected label text value changes on clock `MouseOver`
- Verifies that the connected label text value changes on clock `MouseMove`
### 2. Click Behavior Test
- Verifies that the connected label text value changes on clock `MouseDown`
- Verifies that the connected label text value changes on clock `MouseUp`
#### Note
- None.

----------

## RadioButton Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** 6 Radiobuttons

### 1. Initialization and Pane Verification Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Cicking RadioButton Choices 
- Verifies that 6 radiobuttons are available
- Verifies that the 6 radiobuttons can be clicked 
- Return the value/choices of the radiobuttons

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## Scrollbar Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Clicking and dragging 

### 1. Initialization and Pane Verification Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Scrollbar Interaction 
- Clicking the forward arrow button
- Clicking the backward arrow button 
- Dragging the scrollbar thumb 

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## Scrollpane + Treeview Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Clicking, dragging, expanding treeview 

### 1. Initialization Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Treeview Expansion 
- Expanding various nodes in the treeview such as apps, kardia, sys, and tests

### 3. ScrollPane (or Scrollbar) Functionality
- Verifies that the down arrow scrolls pane down 
- Verifies that the up arrow scrolls pane up 
- Verifies that dragging the thumb scrolls the pane down 

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## Textarea Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Clicking, inputting text into textarea, read only set to yes and no 

### 1. Initialization Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Textarea Interaction
- Verifies of locating the primary text area 
- Verifies of clicking the primary text area 
- Verifies sending a long text string
- Verifies that the textarea can be cleared 
- Verifies that clicking the second text area does nothing

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------


## Textbutton Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Clicking buttons, focused cursor 

### 1. Initialization Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Button 1 Functionality 
- Verifies of locating the first button 
- Verifies of highlighting the first button 
- Verifies that the button is clicked
- Verifies that the alert handling is done well 

### 2. Button 2 Functionality 
- Verifies of locating the second button 
- Verifies of highlighting the second button 
- Verifies that the button is clicked
- Verifies that the alert handling is done well 

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.

----------

## Dropdown Test
### Properties Tested
- `hilight`
- `bgcolor`
- `fieldname`
- `mode`: objectsource

### 1. Click Behavior Test
- Verifies that clicking the dropdown item updates the value
#### Note
- Property `hilight` is stated as `highlight` on the web document but only `hilight` works.

----------

## Menu Test
### Properties Tested
- **style**: `icon`, `label`, `row_height`, `bgcolor`, `highlight_bgcolor`, `active_bgcolor`, `direction`
- `popup`: yes, no

### 1. Mouse over Behavior Test
- Test if mouse over expands menu
- Verifies highlight value change
### 2. Click Behavior Test
- Verifies that click behavior opens up a child window, expands if it is a menu, and updates connected label value
### 3. Right Click Popup
- Verifies menu popup appears on right click
#### Note
- None.

----------

## CMPDECL Part 2 Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Clicking buttons, focused cursor, pulling data from csv file, building a table 

### 1. Initialization Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Iteration and Clicking (RadioButtons)
- Verifies of locating the buttons
- Verifies clicking the buttons and printing the choices out
- Verifies that 3 nested layers are working correctly.
  - Widgets tested: widget/cmp-decl, widget/component, widget/radiobuttonpanel
                    widget/connector, widget/parameter, widget/component-decl-event
  - Verifies that nested layer L1 works
  - Verifies tat nested layer L2 works 
  - Verifies that the .app file works

### 2. Table Data Extraction and Formatting 
- Verifies that the .app file works, with a table and a CSV file as the object source
- Widgets tested: widget/osrce, widget/table, widget/repeat, widget/label, widget/table-column 
- Verifies that the headers and rows can be clearly read

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.
- This is part 2 of CMPDECL Test 

----------

## Tab Test
### Properties Tested
- **style**: `border_color`, `border_radius`, `border_style`, `shadow_angle`, `shadow_color`, `shadow_offset`, `shadow_radius`, `background`, `inactive_background`
- `tab_location`: top, bottom, left, right
- `selected_index`
- `selected`

### 1. Tab Click Behavior Test
- Test if current tab changes on tab click

#### Note
- `selected_index` displays both the first tab and indexth tab as selected
- could not find any example of dynamic tab and was not able to get it work
- tab page with `visible=0` property is still visible

----------

## Tab Test
### Properties Tested
- `widget_class`

### No selenium test
- Verified that template with button successfully appears on the app

#### Note
- Tried to create a hbox template containing buttons and the buttons were not clickable.

----------

## Timer + Button Test
### Properties Tested
- **Page readiness:** `document.readyState`
- **Element Presence:** `<body>`
- **JavaScript Initialization:** `pg_isloaded`
- **Component State:** Focused cursor, clicking, inputting text into tet area, retrieving HTML components, `auto_start`, `auto_reset`

### 1. Initialization Test 

- Verifies that the browser has fully loaded the page's Document Object Model (DOM)
- Confirms that the application's core framework and the specific ChildWindow widget have initialized successfully in JavaScript

### 2. Coordinated Timer, Type and Clear Action 
- Verifies that auto_start and auto_reset properties works
- Verifies that timer is clicked to start a 5s countdown
- Verifies that text area is found post click
- Verifies that text area is able to receive random text for 4 seconds
- Verifies that text area can be cleared
- Verifies that, after 5 seconds, the button is ready to be clicked again (Click Me Again)

### 3. Redo Sequence After Button Reset (Click Me Again)
- Verifies that timer is clicked to start a 5s countdown (now is Click Me Again)
- Verifies that text area is found post click
- Verifies that text area is able to receive random text for 4 seconds
- Verifies that text area can be cleared
- Verifies that, after 5 seconds, the button is ready to be clicked again (Click Me Again)

### 4. Check HTML Elements After Button Reset 
- Verifies that HTML is present 
- Verifies that content length (characters) can be retrieved 
- Verifies that the page title and message is displayed correctly 

#### Note
- All interaction and closing tests are dependent on the successful completion of the Initialization Test.
- This is a combination of a button, timer, and HTML widget test 

----------
