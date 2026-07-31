import sys
import os
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QStackedWidget, QTextEdit, QPushButton, QLabel, QShortcut, QFrame, QGridLayout
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QKeySequence, QFont, QColor, QTextCharFormat, QTextCursor

from pages import (
    InitializationPage, ModeSelectionPage, ActiveProcessPage
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
        self.page_active = ActiveProcessPage()
        
        self.stacked_widget.addWidget(self.page_init)       # Index 0
        self.stacked_widget.addWidget(self.page_mode)       # Index 1
        self.stacked_widget.addWidget(self.page_active)     # Index 2
        
        # Set exact stretch ratios for a responsive layout
        self.main_layout.setStretchFactor(self.header_bar, 0)         # Fit to content
        self.main_layout.setStretchFactor(self.stacked_widget, 1)     # Fill remaining height
        
        # ================== Signals & State ==================
        self.page_init.map_selected.connect(self.on_map_selected)
        self.page_mode.nav_selected.connect(lambda: self.switch_page(2, "nav"))
        self.page_mode.auto_map_selected.connect(lambda: self.switch_page(2, "auto_map"))
        self.page_mode.manual_map_selected.connect(lambda: self.switch_page(2, "manual_map"))
        self.page_active.stop_clicked.connect(self.go_back)
        
        self.history = []
        self.current_map = ""
        
    def switch_page(self, index, mode=None):
        current = self.stacked_widget.currentIndex()
        self.history.append(current)
        self.stacked_widget.setCurrentIndex(index)
        self.btn_back.show()
        
        self.pill_status.hide()
        
        if index == 1:
            self.lbl_title.setText(f"MAP: {self.current_map.upper()}")
        elif index == 2:
            self.page_active.set_state(mode)
            if mode == "nav":
                self.lbl_title.setText(f"AUTONOMOUS NAVIGATION - MAP: {self.current_map.upper()}")
                self.pill_status.setText("NAV ACTIVE")
                self.pill_status.setProperty("class", "statusBadge")
            elif mode == "auto_map":
                self.lbl_title.setText(f"AUTO SLAM MAPPING - MAP: {self.current_map.upper()}")
                self.pill_status.setText("MAPPING")
                self.pill_status.setProperty("class", "statusBadge")
            elif mode == "manual_map":
                self.lbl_title.setText(f"MANUAL SLAM MAPPING - MAP: {self.current_map.upper()}")
                self.pill_status.setText("TELEOP")
                self.pill_status.setProperty("class", "statusBadgeTeleop")
            self.pill_status.show()
            
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
            elif prev == 1:
                self.lbl_title.setText(f"MAP: {self.current_map.upper()}")
                self.pill_status.hide()
            
    def on_map_selected(self, map_name):
        self.current_map = map_name
        self.page_mode.set_map_name(map_name)
        self.switch_page(1)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.showFullScreen()
    sys.exit(app.exec_())
