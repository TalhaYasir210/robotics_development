# Page 1
Create a full-screen, highly optimized PyQt (PyQt6 preferred) desktop application UI that exactly matches the provided reference image, verbatim named "Flow1.png".

Global Window Settings:

    The application must launch in full-screen mode using showFullScreen().

    The main widget must use a QVBoxLayout with setContentsMargins(0, 0, 0, 0) and setSpacing(0) to ensure edge-to-edge rendering.

    Global background color: #FFFFFF (White).

    Default Font: Set a clean, modern sans-serif font (like Arial, Helvetica, or system default) globally.

Section 1: Top Navigation Bar (Fixed Header)

    Container: A QFrame with a fixed height of 60px.

    Styling: Background #FFFFFF, with a bottom border of 1px solid #D3D3D3.

    Layout: QHBoxLayout with left and right margins of 24px and a center alignment constraint.

    Center Element (Title): A QLabel with the text "MAP SELECTION". Positioned exactly in the center of the layout.

        Style: Font size 14px, bold (font-weight 600), letter spacing 2px, color #6C757D (Slate Gray).

    Right Element (Badge): A QLabel or disabled QPushButton placed at the far right.

        Dimensions: Fixed width 110px, fixed height 30px.

        Style: Text "ROS 2 Jazzy", background color #EAF8F5, border 1px solid #57C1A3, text color #2A9D8F, border-radius 5px, font size 12px, bold. Align text to the center.

Section 2: Main Content Area (Dynamic Middle)

    Container: A QFrame that expands to fill all available middle space (use a layout stretch factor of 1).

    Layout: QVBoxLayout with Qt.AlignmentFlag.AlignCenter so all elements inside are perfectly centered vertically and horizontally.

    Element 1 (Main Heading): A QLabel with the text "On which map do you want to navigate the Bot?".

        Style: Font size 32px, bold (font-weight 700), color #212529 (Dark Gray/Black).

        Positioning: Add a bottom margin of 12px.

    Element 2 (Subheading): A QLabel with the text "Choose from the pre-loaded maps available on this robot".

        Style: Font size 18px, color #6C757D (Gray).

        Positioning: Add a bottom margin of 48px.

    Element 3 (Dropdown/Combo Box): A QComboBox.

        Dimensions: Fixed width 400px, fixed height 54px.

        Style: Text "Select Map", background #FFFFFF, border 1px solid #CED4DA, border-radius 8px, padding-left 16px, font size 16px, color #495057.

        Arrow Customization: Use Qt Style Sheets (QSS) to change the dropdown arrow to a custom SVG or colored chevron matching #2A9D8F (Teal), and remove the default Windows/Mac visual styles so it looks like a clean, modern web input.

Section 3: Logger (Fixed Footer)

    Container: A QFrame positioned at the bottom.

    Dimensions: Fixed height of 280px (or exactly 25% of the screen height if using stretch factors).

    Layout: QVBoxLayout with setContentsMargins(0, 0, 0, 0) and setSpacing(0).

    Sub-section A (Logger Header): A QFrame with fixed height 36px.

        Style: Background #F4F5F7, border-top 1px solid #D3D3D3, border-bottom 1px solid #D3D3D3.

        Content: A QLabel with text "LOGGER". Font size 11px, bold, letter spacing 1px, color #495057. Left-aligned with a left margin of 24px.

    Sub-section B (Console Output): A QTextEdit (set to read-only) or QPlainTextEdit.

        Style: Background #F4F5F7, border none, padding 16px.

        Font: Strict Monospace font (e.g., Courier New, Consolas), font size 13px.

        Text Colors: Populate it with dummy text matching the image. Set the timestamps and standard text to #6C757D (Gray). Use rich text formatting to color the word INFO to #2A9D8F (Teal) and DEBUG to #ADB5BD (Light Gray).

Optimization & Code Quality Requirements:

    Use standard PyQt layout stretch factors (addStretch) carefully if absolute fixed pixels behave poorly on different 1080p/4k monitors, but strictly adhere to the heights for the Top Bar (60px) and Dropdown (54px x 400px).

    Keep the UI thread responsive; construct the components cleanly using Object-Oriented principles (e.g., separate classes for TopBar, MainContent, and LoggerArea).

    Include all necessary QSS (Qt Style Sheets) inside the script as a multi-line string variable to ensure the precise colors, borders, and paddings are applied correctly.

    Extend the previous PyQt UI code to precisely style the QComboBox when it is clicked (the "on" or "active" state) and style the dropdown menu list to perfectly match the provided reference image "Flow2.png".

1. Main Dropdown Button (Active State):

    Trigger: When the QComboBox is clicked or focused (use QSS pseudo-states like QComboBox:on or QComboBox:focus).

    Border: Change the border to a thicker teal color to indicate focus: 2px solid #2A9D8F.

    Arrow: The custom chevron/arrow icon must flip to point upwards when the menu is open.

2. Dropdown List Menu (QAbstractItemView):

    Container Styling:

        Background color: #FFFFFF.

        Border: 1px solid #CED4DA.

        Border-radius: 8px (Ensure the corners of the popup menu are rounded, which may require setting QComboBox::drop-down properties and window flags depending on the OS).

        Remove default OS outlines or focus borders from the list.

    Items (Populate the list): Add exactly two items to the combobox: "Office" and "Warehouse".

    Item Styling (QComboBox QAbstractItemView::item):

        Height: Set the minimum height of each item to 50px.

        Padding: padding-left: 16px; to perfectly align with the "Select Map" text in the main box.

        Font: 16px, color #212529.

        Hover State: When an item is hovered (:hover), change its background to a very faint gray (e.g., #F8F9FA) so the user knows it's selectable.

    Separator/Borders between items:

        Ensure there is a visible 1px solid #E9ECEF (light gray) horizontal line separating "Office" and "Warehouse". Hint for AI: In PyQt, this is often best achieved by setting a bottom border on the items in the QAbstractItemView, but removing it for the last item.

3. Implementation Constraint:

    Provide the updated Python code specifically focusing on the advanced QSS string required to achieve these rounded dropdown list corners and active border states, as native PyQt combobox popups can be notoriously stubborn to style. If a custom QListView needs to be set as the combobox's view via setView(QListView()) to apply these styles, please include that in the Python logic.

    Update the PyQt UI layout to handle the state when a map option (e.g., "Office") is selected from the dropdown, rendering the selected value and dynamically displaying the "Confirm Map" action button underneath, matching Flow3.png.
1. Dynamic State Logic & Event Handling

    Initial State (No Selection / Default): The QComboBox displays "Select Map" with no selection actioned, and the "Confirm Map" button is either hidden (setVisible(False)) or disabled.

    Selected State Trigger: Connect the currentTextChanged or currentIndexChanged signal of the QComboBox. When a valid map option (e.g., "Office" or "Warehouse") is selected:

        The QComboBox updates its main display text to reflect the selected map.

        The dropdown menu closes, returning the border to its default clean state.

        The "Confirm Map →" button becomes visible and active directly below the dropdown.

2. "Confirm Map" Button Specifications

    Widget: QPushButton

    Dimensions: Fixed width 200px, fixed height 48px.

    Positioning & Alignment:

        Center horizontally inside the main middle container (QVBoxLayout).

        Positioned directly below the QComboBox with a vertical margin/spacing of 20px.

    Text & Icon:

        Text: "Confirm Map →" (or "Confirm Map" paired with an aligned right-arrow icon/Unicode →).

        Alignment: Center-aligned.

    Styling (QSS):
    CSS

    QPushButton#confirmMapBtn {
        background-color: #1D9A85;
        color: #FFFFFF;
        font-size: 16px;
        font-weight: 600;
        border-radius: 8px;
        border: none;
    }
    QPushButton#confirmMapBtn:hover {
        background-color: #168A79;
        cursor: pointer;
    }
    QPushButton#confirmMapBtn:pressed {
        background-color: #127365;
    }

3. QComboBox Selected Display Refinements

    Default State Style (Post-Selection):

        Border: 1px solid #CED4DA (or light green border 1px solid #1D9A85 if active).

        Text: Set color to #212529 (Dark gray), font size 16px.

        Chevron Arrow: Pointing downwards (v), color #1D9A85.

4. Layout Structure Check
Plaintext

[ Main Content Frame (QVBoxLayout - Centered) ]
 ├── QLabel ("On which map do you want to navigate the Bot?") [32px, Bold]
 ├── QLabel ("Choose from the pre-loaded maps available on this robot") [18px, Gray]
 ├── QComboBox [Width: 400px, Height: 54px]
 └── QPushButton ("Confirm Map →") [Width: 200px, Height: 48px, Top Spacing: 20px]

Ensure the layout updates smoothly without layout shifts or jumpy resizing when the button transitions to visible.

# page 2

Here is the precise prompt for Antigravity to implement the new "Action Selection" view shown in the reference image, verbatim named "Flow4.png".
Prompt for Antigravity: Action Selection Screen (Flow4.png)

Goal: Create the next view in the PyQt application, representing the "Action Selection" screen ("Flow4.png"). It is highly recommended to use a QStackedWidget for the main central area to smoothly switch between the "Map Selection" view and this new "Action Selection" view without destroying the fixed Top Bar and Logger, though the Top Bar contents will need updates.
1. Top Navigation Bar (Updates)

    Left Element (New Back Button): Add a QPushButton aligned to the left side of the Top Bar.

        Text: ‹ Back (Use a standard left-pointing angle quotation mark or SVG icon for the arrow).

        Dimensions: Fixed width 84px, fixed height 36px.

        Style: Background #FFFFFF, border 1px solid #CED4DA, border-radius 6px, font size 14px, color #212529, font-weight 500.

    Center Element (Updated Title): Update the QLabel text to "MAP: OFFICE".

        Style: Maintain previous style (font size 14px, bold, letter spacing 2px, color #6C757D).

    Right Element: Maintain the "ROS 2 Jazzy" badge exactly as it was.

2. Main Content Area (Action Selection View)

    Container: A new QFrame acting as Page 2 in the central QStackedWidget.

    Layout: QVBoxLayout with Qt.AlignmentFlag.AlignCenter to perfectly center all contents.

    Element 1 (Main Heading): A QLabel with the text "Do you want to do Navigation or Mapping?".

        Style: Font size 32px, bold (font-weight 700), color #212529 (Dark Gray/Black).

        Positioning: Bottom margin of 16px.

    Element 2 (Dynamic Subheading): A layout containing rich text or two side-by-side QLabels to display "Active map: Office".

        Text Part 1 ("Active map: "): Font size 18px, color #6C757D.

        Text Part 2 ("Office"): Font size 18px, color #1D9A85 (Teal), use a monospaced or distinct bold font to highlight the variable.

        Positioning: Bottom margin of 48px.

    Element 3 (Action Buttons Container): A QHBoxLayout to hold the two main action buttons side-by-side.

        Alignment: Center alignment, with a spacing of 20px between the two buttons.

3. Action Buttons (Inside the QHBoxLayout)

    Button 1 (Autonomous Navigation): A QPushButton.

        Text: "Autonomous Navigation"

        Dimensions: Fixed width 280px, fixed height 54px.

        Style: Background color #1D9A85 (Teal), text color #FFFFFF, border none, border-radius 8px, font size 16px, bold (font-weight 600).

        Hover State: Background #168A79.

    Button 2 (SLAM Mapping Dropdown): A QPushButton (or QToolButton with a popup menu).

        Text: "SLAM Mapping v" (Use a downward chevron icon).

        Dimensions: Fixed width 260px, fixed height 54px.

        Style: Background color #648D6C (Muted/Olive Green), text color #FFFFFF, border none, border-radius 8px, font size 16px, bold (font-weight 600).

        Hover State: Background #557A5D.

4. Logger Area (Footer)

    Maintain the fixed footer logger precisely as built in previous steps.

    Update the dummy text to reflect the new ROS 2 node initializations shown in the reference image (e.g., map_server Map loaded successfully, amcl Initializing Monte-Carlo localization, slam_toolbox Ready). Keep the word INFO styled in teal #1D9A85 and DEBUG in light gray #ADB5BD.


Here is the precise prompt for Antigravity to implement the active dropdown state for the "SLAM Mapping" button shown in the reference image, verbatim named "Flow6.png".
Prompt for Antigravity: SLAM Mapping Dropdown State (Flow6.png)

Goal: Extend the Action Selection view (Page 2 of the QStackedWidget) to handle the active state when the "SLAM Mapping" button is clicked. This requires implementing a custom multi-line dropdown menu that precisely matches "Flow6.png".
1. The SLAM Mapping Button (Active State)

    Trigger: When the user clicks the "SLAM Mapping" button.

    Icon Update: The downward chevron (v) must update to an upward-pointing chevron (^) to indicate the menu is open.

    Style Persistence: Maintain the background color #648D6C (Olive Green) while the menu is open.

2. Custom Dropdown Menu (Implementation Strategy)

    PyQt Constraint: Since standard QMenu or QComboBox items do not easily support two distinct lines of text (a title and a subtitle) with different font weights, you must implement this as a custom floating QFrame (positioned absolutely relative to the button) OR use a QMenu utilizing QWidgetAction to insert custom widgets for each row.

    Menu Container Specifications:

        Width: Exactly matches the width of the SLAM Mapping button (260px).

        Position: Directly below the SLAM Mapping button, flush with its bottom edge or with a tiny 4px gap.

        Style: Background #FFFFFF, border 1px solid #CED4DA, border-radius 8px. (If using a shadow effect, apply a very subtle QGraphicsDropShadowEffect).

        Layout: QVBoxLayout with 0px spacing and 0px margins to allow the internal items to touch the edges.

3. Menu Items (The Options)

Create two clickable areas (custom widgets or custom QPushButtons) inside the menu layout.

    Option 1: Auto Mapping

        Layout: QVBoxLayout inside a clickable container.

        Padding: Top/Bottom 12px, Left/Right 16px.

        Main Text: "Auto Mapping". Font size 15px, bold (font-weight 600), color #212529.

        Subtext: "Autonomous frontier exploration". Font size 12px, color #6C757D, top margin of 4px.

        Bottom Border: Add a 1px solid #E9ECEF border to the bottom of this item to act as a separator.

    Option 2: Manual Mapping

        Layout: QVBoxLayout inside a clickable container.

        Padding: Top/Bottom 12px, Left/Right 16px.

        Main Text: "Manual Mapping". Font size 15px, bold (font-weight 600), color #212529.

        Subtext: "Teleoperate with keyboard". Font size 12px, color #6C757D, top margin of 4px.

        Bottom Border: None (since it's the last item).

    Hover State for Items:

        When the user hovers over either the "Auto Mapping" or "Manual Mapping" container, the background of that specific item should change to a very faint gray (e.g., #F8F9FA) and the cursor should change to Qt.CursorShape.PointingHandCursor. Ensure the border radius of the hover state respects the outer container's 8px rounded corners (top corners for item 1, bottom corners for item 2).

4. Logger text (Unchanged)

    The logger text remains largely identical to the previous screen. No structural changes are needed to the logger footer.

# page 3

Here is the precise prompt for Antigravity to implement the "Autonomous Navigation Mode" screen shown in the reference image, verbatim named "Flow5.jpg".
Prompt for Antigravity: Autonomous Navigation Mode (Flow5.jpg)

Goal: Create the third view (Page 3) in the QStackedWidget representing the "Autonomous Navigation Mode" screen. This screen features two large placeholder panels for ROS 2 visualizer feeds (RViz and Gazebo), top instruction text, and action buttons.
1. Top Navigation Bar (Updates for Page 3)

    Left Element (Back Button): Keep the ‹ Back button exactly as defined in Page 2.

    Center Element (Updated Title): Update the QLabel text to "AUTONOMOUS NAVIGATION MODE". Maintain the previous font styling (14px, bold, letter spacing 2px, #6C757D).

    Right Element (Dual Badges): Replace the single right badge with a QHBoxLayout containing two badges side-by-side.

        Badge 1 (New): Text "NAV ACTIVE".

        Badge 2 (Existing): Text "ROS 2 Jazzy".

        Badge Styling (Both): Fixed height 30px, padding 0 12px, background #EAF8F5, border 1px solid #57C1A3, text color #2A9D8F, border-radius 5px, font size 11px, bold, center-aligned.

2. Main Content Area (Navigation View)

    Container: A new QFrame acting as Page 3 in the QStackedWidget.

    Main Layout: QVBoxLayout with a top margin of 24px and bottom margin of 24px, center-aligned horizontally.

    Element 1 (Instruction Subtitle): A QLabel using Rich Text: "Set the destination pose by clicking <b>2D Nav Goal</b> in the RViz2 panel".

        Style: Font size 16px, color #495057 (Dark Gray), center-aligned. Bottom margin of 16px.

3. Video Feed Panels (The Core Feature)

    Layout Container: A QHBoxLayout to hold the left and right feed panels side-by-side. Give it a fixed height of roughly 400px to 450px (or expand to fill available space) and a spacing of 20px.

    Panel Structure (Apply to both Left and Right):

        Use a QFrame for the outer card.

        Card Style: Background #FFFFFF, border 1px solid #CED4DA, border-radius 8px.

        Inner Layout: QVBoxLayout with 0px margins.

        Header Bar: A QLabel at the top of the card with padding 12px. Font size 12px, bold, color #6C757D, letter spacing 1px.

        Feed Area: A QLabel or QFrame underneath the header that fills the rest of the card. Give it a dark/gray background to simulate a video feed (e.g., #E9ECEF or #343A40 with an embedded placeholder image). Set border-bottom-left-radius and border-bottom-right-radius to 8px so the feed respects the card's rounded corners.

    Left Panel Specifics:

        Header Text: "MAP VIEW — RVIZ2 / SLAM"

        Feed Styling: Simulate the RViz grid. (For the UI mock, just set the background to a light grid pattern or a static image if possible, otherwise a solid #F8F9FA with a dark gray border).

        Overlay Tags: Add small floating tags (using absolute positioning move() or a nested grid layout) in the corners:

            Bottom-left tag: "RViz2 - /map" (Dark background, white text).

    Right Panel Specifics:

        Header Text: "LIVE CAMERA — GAZEBO"

        Feed Styling: Simulate the 3D camera. Set background to a slate/blue-gray gradient or solid #95A5A6.

        Overlay Tags:

            Top-right tag: "● LIVE - 30 HZ" (Red background #D9534F, white text, border-radius 4px).

            Top-left tag: Simulated velocity metrics (vel_x: 0.50 m/s).

            Bottom-left tag: "Gazebo - /camera/rgb" (Dark background).

4. Control Buttons (Bottom of Main Area)

    Layout: A QHBoxLayout centered horizontally beneath the video feeds, with a top margin of 24px and a spacing of 16px.

    Button 1 (Pause Navigation): * Widget: QPushButton with text "Pause Navigation".

        Dimensions: Fixed width 240px, height 48px.

        Style: Background #5A7BC4 (Soft Royal Blue), text color #FFFFFF, border none, border-radius 8px, font size 16px, bold. Hover background #4A6BB4.

    Button 2 (Cancel Navigation):

        Widget: QPushButton with text "Cancel Navigation".

        Dimensions: Fixed width 240px, height 48px.

        Style: Background #CF6666 (Soft Red/Coral), text color #FFFFFF, border none, border-radius 8px, font size 16px, bold. Hover background #BF5656.

5. Logger Area (Footer Updates)

    Maintain the layout and height of the Logger exactly as before.

    Update the mock terminal text to reflect the active navigation state. Example text to insert:
    Plaintext

    [13:39:10.547] INFO  nav2_bt_navigator Autonomous navigation mode ACTIVE
    [13:39:14.245] INFO  amcl Particle filter converged - estimated confidence: 0.947
    [13:39:14.947] INFO  rviz2 Waiting for 2D Nav Goal via RViz interactive tool...
    [13:39:15.648] DEBUG controller_server DWB local planner initialized, max_vel_x=0.50 m/s
    [13:39:16.348] DEBUG sensor_fusion /scan @ 10 Hz | /camera/rgb/image_raw @ 30 Hz
    [13:39:17.048] INFO  nav2_bt_navigator Goal pose received: x=4.21 y=2.85 θ=0.00 rad
    [13:39:17.748] INFO  global_planner Path computed: 47 waypoints, distance=12.3 m

    Ensure INFO is teal #1D9A85 and DEBUG is gray #ADB5BD

# page 4

Create a new view (Page 4) in the QStackedWidget for the "Auto SLAM Mapping" state. This layout is structurally very similar to the Autonomous Navigation view, featuring the dual video feeds, but it lacks bottom control buttons and updates several text labels and badges to match "Flow7.jpg".
1. Top Navigation Bar (Updates for Page 4)

    Left Element (Back Button): Keep the ‹ Back button exactly as defined in previous states.

    Center Element (Updated Title): Update the QLabel text to "AUTO SLAM MAPPING".

        Style: Maintain previous font styling (14px, bold, letter spacing 2px, #6C757D).

    Right Element (Dual Badges):

        Badge 1 (Updated): Change text to "MAPPING". Background #EAF8F5, border 1px solid #57C1A3, text color #2A9D8F, border-radius 5px, font size 11px, bold.

        Badge 2 (Existing): Text "ROS 2 Jazzy". Same styling as Badge 1.

2. Main Content Area (Auto Mapping View)

    Container: A new QFrame acting as Page 4 in the QStackedWidget.

    Main Layout: QVBoxLayout with top/bottom margins of 24px, center-aligned horizontally.

    Element 1 (Instruction Subtitle): A QLabel with the text: "Robot is autonomously exploring and building the map — no operator input required".

        Style: Font size 16px, color #495057 (Dark Gray), center-aligned. Bottom margin of 16px.

3. Video Feed Panels (Updates from Navigation State)

    Layout Container: Reuse the QHBoxLayout structure from the Navigation state (fixed height ~400px to 450px, spacing 20px).

    Left Panel (RViz/Map Updates):

        Header Text: Update to "MAP VIEW — RVIZ2 / SLAM (BUILDING)".

        Feed Visuals: Maintain the dark gray background, but if using placeholders, suggest a graphic showing a white "explored" area over a gray grid to simulate active SLAM.

        Overlay Tags: Keep "RViz2 - /map" at the bottom left and "28x22 · 0.05m/px" at the top right.

    Right Panel (Gazebo Camera):

        Header Text: "LIVE CAMERA — GAZEBO" (Unchanged).

        Feed Visuals & Overlay Tags: Completely identical to the Navigation state (vel_x, vel_z, LIVE - 30 HZ, and Gazebo - /camera/rgb).

4. Control Buttons (Crucial Removal)

    No Action Buttons: Unlike the Navigation mode, do not include any bottom control buttons (like Pause or Cancel). The space below the video feeds should simply remain empty margin connecting directly to the Logger.

5. Logger Area (Footer Updates)

    Maintain the layout and height of the Logger area.

    Update the mock terminal text to reflect the active SLAM mapping and explore_lite node initializations. Replace the content with the following dummy text:
    Plaintext

    [13:39:26.707] INFO  slam_toolbox Auto SLAM mapping mode ACTIVE
    [13:39:27.606] INFO  slam_toolbox Serialization ON - output: /maps/slam_session_001
    [13:39:28.306] DEBUG laser_scan_matcher ICP convergence achieved - avg residual: 0.003 m
    [13:39:29.007] INFO  occupancy_grid Resolution 0.05 m/px | Origin [0.000, 0.000, 0.000]
    [13:39:29.706] INFO  explore_lite Frontier exploration initiated
    [13:39:30.407] DEBUG explore_lite Active frontiers detected: 14
    [13:39:31.107] INFO  move_base Navigating to frontier [4.21 m, 1.87 m]

    Ensure INFO remains colored teal #1D9A85 and DEBUG remains gray #ADB5BD.

# page 5
Here is the precise prompt for Antigravity to implement the "Manual SLAM Mapping" screen shown in the reference image, verbatim named "Flow8.jpg".
Prompt for Antigravity: Manual SLAM Mapping Mode (Flow8.jpg)

Goal: Create a new view (Page 5) in the QStackedWidget for the "Manual SLAM Mapping" state. This layout reuses the dual video feeds from previous screens but introduces a central, on-screen D-pad style keyboard controller panel underneath the feeds, replacing the standard action buttons.
1. Top Navigation Bar (Updates for Page 5)

    Left Element (Back Button): Keep the ‹ Back button unchanged.

    Center Element (Updated Title): Update the QLabel text to "MANUAL SLAM MAPPING". Maintain the standard font styling (14px, bold, letter spacing 2px, #6C757D).

    Right Element (Dual Badges):

        Badge 1 (New): Text "TELEOP". Background #E9F2FE (Light Blue), border 1px solid #A3C5F5, text color #4B73D0 (Deep Blue), border-radius 5px, font size 11px, bold, center-aligned.

        Badge 2 (Existing): Text "ROS 2 Jazzy". Keep standard teal styling.

2. Main Content Area (Manual Mapping View)

    Container: A new QFrame acting as Page 5 in the QStackedWidget.

    Main Layout: QVBoxLayout with top/bottom margins of 24px, center-aligned horizontally.

    Element 1 (Instruction Subtitle): A QLabel with the text: "Drive the robot manually using keyboard controls to build the map".

        Style: Font size 16px, color #495057 (Dark Gray), center-aligned. Bottom margin of 16px.

3. Video Feed Panels

    Reuse the exact QHBoxLayout and inner card structures from the Auto SLAM Mapping state. The headers ("MAP VIEW — RVIZ2 / SLAM (BUILDING)" and "LIVE CAMERA — GAZEBO") and the internal placeholders remain identical.

4. Teleop Keyboard Controls (The D-Pad Panel)

    Container: Directly below the video feeds, create a QFrame centered horizontally.

        Dimensions: Fixed width ~320px, fixed height ~220px.

        Style: Background color #F1F3F5 (Light Gray), border 1px solid #CED4DA, border-radius 12px.

    Panel Title: A QLabel at the top of this container.

        Text: "TELEOP KEYBOARD CONTROLS".

        Style: Font size 11px, bold, letter spacing 1px, color #6C757D, center-aligned, with a top margin of 16px.

    Keypad Layout: Use a QGridLayout to arrange the D-pad movement keys to control the bot manually.

        The Keys: Create four QLabel or disabled QPushButton widgets for "I", "J", "K", "L".

        Key Styling: Background #FFFFFF, border 1px solid #CED4DA, border-radius 6px, font size 18px, bold, color #212529. Add a subtle drop shadow (QGraphicsDropShadowEffect) to simulate a physical keyboard keycap. Dimensions: 48px by 48px.

        Positions in Grid:

            Row 0, Col 1: "I" key.

            Row 1, Col 1: Label "Move Forward" (Font size 11px, color #6C757D, center-aligned).

            Row 2, Col 0: "J" key.

            Row 2, Col 1: "K" key.

            Row 2, Col 2: "L" key.

            Row 3, Col 0: Label "Rotate Left" (Font size 11px).

            Row 3, Col 1: Label "Stop" (Font size 11px).

            Row 3, Col 2: Label "Rotate Right" (Font size 11px).

        Spacing: Ensure horizontal and vertical spacing in the grid gives it a comfortable, controller-like feel.

5. Logger Area (Footer Updates)

    Maintain the fixed Logger footer.

    Update the mock terminal text to reflect the active teleoperation nodes. Use this dummy text:
    Plaintext

    [13:39:34.776] INFO  slam_toolbox Manual SLAM mapping mode ACTIVE
    [13:39:35.474] INFO  teleop_twist_keyboard Ready — listening on /cmd_vel topic
    [13:39:36.176] DEBUG laser_scan_matcher ICP convergence achieved - avg residual: 0.004 m
    [13:39:36.875] INFO  occupancy_grid Resolution 0.05 m/px | Origin [0.000, 0.000, 0.000]
    [13:39:37.575] INFO  slam_toolbox Serialization ON — output: /maps/manual_session_001

    Ensure INFO remains colored teal #1D9A85 and DEBUG remains gray #ADB5BD.