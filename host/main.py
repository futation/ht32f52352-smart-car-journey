# 主程序入口
import tkinter as tk
from tkinter import ttk
from chat_gui import ChatGUI
from serial_assistant import SerialAssistant
from path_planner import AStarPlanner
from ble_interface import BLEApp
from book_converter import BookConverter


class MainApplication(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("智能书库导航系统 v3.0")
        self.geometry("1200x800")

        # 创建顶部导航栏
        self.create_navbar()

        # 创建功能模块容器
        self.container = ttk.Notebook(self)
        self.container.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=(8,10))

        # 初始化所有模块
        self.modules = {
            "对话助手": ChatGUI(self.container),
            "书籍转换": BookConverter(self.container),
            "路径规划": AStarPlanner(self.container),
            "蓝牙连接": BLEApp(self.container),
            "串口助手": SerialAssistant(self.container)
        }

        # 添加所有模块到容器
        for name, module in self.modules.items():
            if hasattr(module, 'canvas'):
                module.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            self.container.add(module, text=name)

        self.setup_module_links()


    def create_navbar(self):
        """创建顶部导航栏"""
        navbar = ttk.Frame(self, height=20)
        navbar.pack(side=tk.TOP, fill=tk.X, padx=3, pady=3)

        # 添加组件时使用anchor参数固定位置
        ttk.Label(navbar, text="系统状态：正常", foreground="green").pack(side=tk.LEFT, padx=10, anchor='nw')

        # 右侧按钮使用anchor='ne'
        ttk.Button(navbar, text="切换全屏", command=self.toggle_fullscreen).pack(side=tk.RIGHT, padx=5, anchor='ne')
        ttk.Button(navbar, text="退出系统", command=self.destroy).pack(side=tk.RIGHT, padx=5, anchor='ne')

        # 强制更新布局
        navbar.update_idletasks()

    def setup_module_links(self):
        """设置模块间的通信连接"""
        converter = self.modules["书籍转换"]
        planner = self.modules["路径规划"]
        serial = self.modules["串口助手"]

        # 书籍转换 -> 路径规划
        if hasattr(converter, 'set_conversion_callback'):
            converter.set_conversion_callback(planner.set_coordinate)

        # 路径规划 -> 串口助手
        if hasattr(planner, 'set_code_callback'):
            planner.set_code_callback(serial.update_send_text)

    def toggle_fullscreen(self):
        # 更新Matplotlib画布
        for module in self.modules.values():
            if isinstance(module, AStarPlanner):
                module.canvas.draw()
        self.attributes("-fullscreen", not self.attributes("-fullscreen"))

if __name__ == "__main__":
    app = MainApplication()
    app.mainloop()