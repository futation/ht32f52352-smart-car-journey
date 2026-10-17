# chat_gui.py
import tkinter as tk
from tkinter import scrolledtext, messagebox
from chat_logic import ChatProcessor


class ChatGUI(tk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.processor = ChatProcessor()
        self.init_ui()
        self.display_welcome()
        self.setup_context_menu()  # 新增右键菜单初始化

    def init_ui(self):
        self.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        self.configure(bg="#F0F0F0")

        # 聊天区域增强属性配置
        self.chat_history = scrolledtext.ScrolledText(
            self,
            wrap=tk.WORD,
            state='disabled',
            font=('微软雅黑', 11),
            padx=10,
            pady=10,
            bg="#FAFAFA",
            relief=tk.FLAT,
            selectbackground="#7EC0EE",
            inactiveselectbackground="#7EC0EE",
            exportselection=1,
            insertwidth=0
        )
        self.chat_history.pack(fill=tk.BOTH, expand=True, pady=(0, 10))

        # 输入区域
        input_frame = tk.Frame(self, bg="#F0F0F0")
        input_frame.pack(side=tk.BOTTOM, fill=tk.X)

        self.user_input = tk.Entry(
            input_frame,
            font=('微软雅黑', 12),
            relief=tk.GROOVE,
            borderwidth=2,
            highlightthickness=2,
            highlightcolor="#0084FF",
            highlightbackground="#CCCCCC",
            insertbackground="#0084FF"
        )
        self.user_input.pack(side=tk.LEFT, fill=tk.X, expand=True, ipady=5)
        self.user_input.bind("<Return>", lambda e: self.process_input())

        send_btn = tk.Button(
            input_frame,
            text="发送",
            command=self.process_input,
            bg="#0084FF",
            fg="white",
            font=('微软雅黑', 11, "bold"),
            relief=tk.FLAT,
            padx=20,
            activebackground="#0066CC"
        )
        send_btn.pack(side=tk.RIGHT, padx=(10, 0))

        # 增强样式配置
        self.configure_tags()

    def setup_context_menu(self):
        """新增右键菜单配置"""
        self.context_menu = tk.Menu(self, tearoff=0)
        self.context_menu.add_command(label="复制", command=self.copy_selected)

        # 绑定到所有文本组件
        for widget in [self.chat_history, self.user_input]:
            widget.bind("<Button-3>", self.show_context_menu)

    def show_context_menu(self, event):
        """显示右键菜单"""
        try:
            self.context_menu.tk_popup(event.x_root, event.y_root)
        finally:
            self.context_menu.grab_release()

    def copy_selected(self):
        """复制选中内容"""
        try:
            selected = self.chat_history.get(tk.SEL_FIRST, tk.SEL_LAST)
            self.clipboard_clear()
            self.clipboard_append(selected)
        except tk.TclError:
            pass

    def configure_tags(self):
        """增强标签样式配置"""
        # 用户消息样式
        self.chat_history.tag_config('user',
                                    foreground='blue',
                                    background='#D0F0C0',
                                    justify='right',
                                    lmargin1=60,
                                    lmargin2=20,
                                    rmargin=20,
                                    spacing1=5,
                                    spacing3=8,
                                    selectbackground='#7EC0EE')

        # 助手消息样式
        self.chat_history.tag_config('bot',
                                    foreground='black',
                                    background='#E5E5E5',
                                    justify='left',
                                    lmargin1=20,
                                    lmargin2=20,
                                    rmargin=60,
                                    spacing1=5,
                                    spacing3=8,
                                    selectbackground='#7EC0EE')

        # 空白行样式
        for tag in ['user', 'bot']:
            self.chat_history.tag_config(f"{tag}_start", spacing1=0)
            self.chat_history.tag_config(f"{tag}_end", spacing3=0)

    def display_welcome(self):
        welcome_msg = "您好，我是智能问答助手！\n 我可以帮您：\n 1.查找图书馆的所有在册书籍。 \n 2.根据需要，给您推荐相关的书籍。 \n 3.介绍软件的使用方法。\n 4.查询某对应书籍的位置编码。\n 发送答1 答2 答3 答4 来咨询对应功能。 \n 当然，我也支持基础的聊天功能，不信你发一句“你好”试试。 "
        self.display_message("系统", welcome_msg, "bot")

    def display_message(self, sender, message, msg_type):
        """改进的消息显示方法"""
        self.chat_history.config(state='normal')
        self.chat_history.insert(tk.END, "\n", f"{msg_type}_start")  # 顶部间距
        self.chat_history.insert(tk.END, f" {message} ", msg_type)
        self.chat_history.insert(tk.END, "\n", f"{msg_type}_end")  # 底部间距
        self.chat_history.see(tk.END)
        self.chat_history.config(state='disabled')

    def process_input(self):
        user_text = self.user_input.get().strip()
        if not user_text:
            messagebox.showwarning("提示", "请输入内容")
            return

        self.display_message("你", user_text, "user")
        response = self.processor.get_response(user_text)
        self.display_message("助手", response, "bot")
        self.user_input.delete(0, tk.END)