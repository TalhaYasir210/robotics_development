import sys
import os
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QStackedWidget, QTextEdit, QPushButton, QLabel, QShortcut, QFrame, QGridLayout
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QKeySequence, QFont, QColor, QTextCharFormat, QTextCursor

from pages import (
    InitializationPage, ModeSelectionPage, NavigationActionPage, 
    AutoMappingPage, ManualMappingPage
)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ROS 2 Autonomous Navigation")
        
        # Load Stylesheet
        qss_path = os.path.join(os.path.dirname(__file__), "theme.qss")
        if os.path.exists(qss_path):
            with open(qss_path, "r") as f:
                self.setStyleSheet(f.read())

        self.shortcut_esc = QShortcut(QKeySequence("Esc"), self)
        self.shortcut_esc.activated.connect(self.close)

        self.central_widget = QWidget()
        self.central_widget.setObjectName("mainCanvas")
        self.setCentralWidget(self.central_widget)
        
        self.main_layout = QVBoxLayout(self.central_widget)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(0)
        
        # ================== Section 1: Top Navigation Bar ==================
        self.header_bar = QFrame()
        self.header_bar.setObjectName("headerBar")
        self.header_layout = QGridLayout(self.header_bar)
        self.header_layout.setContentsMargins(24, 0, 24, 0)
        
        # Left (Back Button)
        self.btn_back = QPushButton("‹ Back")
        self.btn_back.setObjectName("btnBack")
        self.btn_back.clicked.connect(self.go_back)
        self.btn_back.hide() # Hidden on first page
        
        # Center (Title)
        self.lbl_title = QLabel("MAP SELECTION")
        self.lbl_title.setObjectName("headerTitle")
        self.lbl_title.setAlignment(Qt.AlignCenter)
        
        # Right (Badges)
        self.pill_layout = QHBoxLayout()
        self.pill_layout.setSpacing(10)
        self.pill_layout.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        
        self.pill_status = QLabel("")
        self.pill_status.hide()
        
        self.pill_ros = QLabel("ROS 2 Jazzy")
        self.pill_ros.setProperty("class", "statusBadge")
        
        self.pill_layout.addWidget(self.pill_status)
        self.pill_layout.addWidget(self.pill_ros)
        
        # Add to header layout using Grid (row 0, columns 0, 1, 2)
        self.header_layout.addWidget(self.btn_back, 0, 0, Qt.AlignLeft | Qt.AlignVCenter)
        self.header_layout.addWidget(self.lbl_title, 0, 1, Qt.AlignCenter)
        self.header_layout.addLayout(self.pill_layout, 0, 2, Qt.AlignRight | Qt.AlignVCenter)
        
        # Equal stretch on columns 0 and 2 keeps column 1 perfectly centered!
        self.header_layout.setColumnStretch(0, 1)
        self.header_layout.setColumnStretch(1, 0)
        self.header_layout.setColumnStretch(2, 1)
        
        self.main_layout.addWidget(self.header_bar)
        
        # ================== Section 2: Main Content Area ==================
        self.stacked_widget = QStackedWidget()
        self.main_layout.addWidget(self.stacked_widget)
        
        # Initialize Pages
        self.page_init = InitializationPage()
        self.page_mode = ModeSelectionPage()
        self.page_nav = NavigationActionPage()
        self.page_auto_map = AutoMappingPage()
        self.page_manual_map = ManualMappingPage()
        
        self.stacked_widget.addWidget(self.page_init)       # Index 0
        self.stacked_widget.addWidget(self.page_mode)       # Index 1
        self.stacked_widget.addWidget(self.page_nav)        # Index 2
        self.stacked_widget.addWidget(self.page_auto_map)   # Index 3
        self.stacked_widget.addWidget(self.page_manual_map) # Index 4
        
        # ================== Section 3: Logger (Fixed Footer) ==================
        self.logger_frame = QFrame()
        # Removed fixed height to allow stretch factor to control sizing
        self.logger_layout = QVBoxLayout(self.logger_frame)
        self.logger_layout.setContentsMargins(0, 0, 0, 0)
        self.logger_layout.setSpacing(0)
        
        # Logger Header
        self.logger_header = QFrame()
        self.logger_header.setObjectName("loggerHeader")
        self.logger_header_layout = QHBoxLayout(self.logger_header)
        self.logger_header_layout.setContentsMargins(0, 0, 0, 0)
        self.logger_title = QLabel("LOGGER")
        self.logger_title.setObjectName("loggerTitle")
        self.logger_header_layout.addWidget(self.logger_title, 0, Qt.AlignLeft | Qt.AlignVCenter)
        self.logger_layout.addWidget(self.logger_header)
        
        # Logger Console
        self.logger_console = QTextEdit()
        self.logger_console.setObjectName("loggerConsole")
        self.logger_console.setReadOnly(True)
        self.logger_layout.addWidget(self.logger_console, stretch=1)
        
        self.main_layout.addWidget(self.logger_frame)
        
        # Set exact stretch ratios for a responsive layout
        self.main_layout.setStretchFactor(self.header_bar, 0)         # Fit to content
        self.main_layout.setStretchFactor(self.stacked_widget, 75)    # 75% of remaining height
        self.main_layout.setStretchFactor(self.logger_frame, 25)      # 25% of remaining height
        
        # ================== Signals & State ==================
        self.page_init.map_selected.connect(self.on_map_selected)
        self.page_mode.nav_selected.connect(lambda: self.switch_page(2))
        self.page_mode.auto_map_selected.connect(lambda: self.switch_page(3))
        self.page_mode.manual_map_selected.connect(lambda: self.switch_page(4))
        
        self.history = []
        self.current_map = ""
        
        self.log_msg("13:39:00.000", "INFO", "GUI Initialized. Awaiting map selection.")
        
    def switch_page(self, index):
        current = self.stacked_widget.currentIndex()
        self.history.append(current)
        self.stacked_widget.setCurrentIndex(index)
        self.btn_back.show()
        
        self.pill_status.hide()
        
        if index == 1:
            self.lbl_title.setText(f"MAP: {self.current_map.upper()}")
            self.logger_console.clear()
            self.log_msg("13:39:05.123", "INFO", "map_server Map loaded successfully")
            self.log_msg("13:39:06.456", "INFO", "amcl Initializing Monte-Carlo localization")
            self.log_msg("13:39:07.789", "INFO", "slam_toolbox Ready")
        elif index == 2:
            self.lbl_title.setText(f"AUTONOMOUS NAVIGATION MODE - MAP: {self.current_map.upper()}")
            self.pill_status.setProperty("class", "statusBadge")
            self.pill_status.setText("NAV ACTIVE")
            self.pill_status.show()
            self.logger_console.clear()
            self.log_msg("13:39:10.547", "INFO", "nav2_bt_navigator Autonomous navigation mode ACTIVE")
            self.log_msg("13:39:14.245", "INFO", "amcl Particle filter converged - estimated confidence: 0.947")
            self.log_msg("13:39:14.947", "INFO", "rviz2 Waiting for 2D Nav Goal via RViz interactive tool...")
            self.log_msg("13:39:15.648", "DEBUG", "controller_server DWB local planner initialized, max_vel_x=0.50 m/s")
            self.log_msg("13:39:16.348", "DEBUG", "sensor_fusion /scan @ 10 Hz | /camera/rgb/image_raw @ 30 Hz")
            self.log_msg("13:39:17.048", "INFO", "nav2_bt_navigator Goal pose received: x=4.21 y=2.85 θ=0.00 rad")
            self.log_msg("13:39:17.748", "INFO", "global_planner Path computed: 47 waypoints, distance=12.3 m")
        elif index == 3:
            self.lbl_title.setText(f"AUTO SLAM MAPPING - MAP: {self.current_map.upper()}")
            self.pill_status.setProperty("class", "statusBadge")
            self.pill_status.setText("MAPPING")
            self.pill_status.show()
            self.logger_console.clear()
            self.log_msg("13:39:26.707", "INFO", "slam_toolbox Auto SLAM mapping mode ACTIVE")
            self.log_msg("13:39:27.606", "INFO", "slam_toolbox Serialization ON - output: /maps/slam_session_001")
            self.log_msg("13:39:28.306", "DEBUG", "laser_scan_matcher ICP convergence achieved - avg residual: 0.003 m")
            self.log_msg("13:39:29.007", "INFO", "occupancy_grid Resolution 0.05 m/px | Origin [0.000, 0.000, 0.000]")
            self.log_msg("13:39:29.706", "INFO", "explore_lite Frontier exploration initiated")
            self.log_msg("13:39:30.407", "DEBUG", "explore_lite Active frontiers detected: 14")
            self.log_msg("13:39:31.107", "INFO", "move_base Navigating to frontier [4.21 m, 1.87 m]")
        elif index == 4:
            self.lbl_title.setText(f"MANUAL SLAM MAPPING - MAP: {self.current_map.upper()}")
            self.pill_status.setProperty("class", "statusBadgeTeleop")
            self.pill_status.setText("TELEOP")
            self.pill_status.show()
            self.logger_console.clear()
            self.log_msg("13:39:34.776", "INFO", "slam_toolbox Manual SLAM mapping mode ACTIVE")
            self.log_msg("13:39:35.474", "INFO", "teleop_twist_keyboard Ready — listening on /cmd_vel topic")
            self.log_msg("13:39:36.176", "DEBUG", "laser_scan_matcher ICP convergence achieved - avg residual: 0.004 m")
            self.log_msg("13:39:36.875", "INFO", "occupancy_grid Resolution 0.05 m/px | Origin [0.000, 0.000, 0.000]")
            self.log_msg("13:39:37.575", "INFO", "slam_toolbox Serialization ON — output: /maps/manual_session_001")
            
        self.style().unpolish(self.pill_status)
        self.style().polish(self.pill_status)
        
    def go_back(self):
        if self.history:
            prev = self.history.pop()
            self.stacked_widget.setCurrentIndex(prev)
            if not self.history:
                self.btn_back.hide()
                self.lbl_title.setText("MAP SELECTION")
                self.pill_status.hide()
                self.logger_console.clear()
                self.log_msg("13:39:00.000", "INFO", "GUI Initialized. Awaiting map selection.")
            elif prev == 1:
                self.lbl_title.setText(f"MAP: {self.current_map.upper()}")
                self.pill_status.hide()
                self.logger_console.clear()
                self.log_msg("13:39:05.123", "INFO", "map_server Map loaded successfully")
            
    def on_map_selected(self, map_name):
        self.current_map = map_name
        self.page_mode.set_map_name(map_name)
        self.switch_page(1)
        
    def log_msg(self, time_str, level, msg):
        # Format: [13:39:34.776] INFO  text
        color = "#1D9A85" if level == "INFO" else "#ADB5BD"
        html = f'<span style="color:#6C757D;">[{time_str}]</span> <span style="color:{color}; font-weight:bold;">{level.ljust(5)}</span> <span style="color:#6C757D;">{msg}</span>'
        self.logger_console.append(html)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.showFullScreen()
    sys.exit(app.exec_())
