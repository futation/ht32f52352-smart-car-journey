# book_converter.py
#书籍转换代码
import tkinter as tk
from tkinter import ttk, scrolledtext
from tkinter import Menu


class BookConverter(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.conversion_library = {
            # 原转换库数据保持不变
            "数学分析": "@01020A01.",
            "抽象代数": "@01020A02.",
            "微分几何": "@01020A03.",
            "实变函数": "@01020A04.",
            "数论导引": "@01020A05.",
            "数学物理方程": "@02020A01.",
            "数值分析": "@02020A02.",
            "最优化理论": "@02020A03.",
            "控制论基础": "@02020A04.",
            "数学建模": "@02020A05.",
            "解析几何": "@03020A01.",
            "微分方程": "@03020A02.",
            "组合数学": "@03020A03.",
            "数理逻辑": "@03020A04.",
            "计算数学": "@03020A05.",
            "运筹学": "@01010A01.",
            "编码理论": "@01010A02.",
            "密码学": "@01010A03.",
            "计算几何": "@01010A04.",
            "数学史": "@01010A05.",
            "数学教育": "@02010A01.",
            "数学文化": "@02010A02.",
            "数学美学": "@02010A03.",
            "数学方法论": "@02010A04.",
            "金融数学": "@02010A05.",
            "保险精算": "@03010A01.",
            "生物数学": "@03010A02.",
            "医学统计": "@03010A03.",
            "工程数学": "@03010A04.",
            "经济数学": "@03010A05.",
        }
        self.setup_ui()
        self.setup_context_menu()
        self.conversion_callback = None  # 新增属性

    def set_conversion_callback(self, callback):
        """设置转换成功后的回调函数"""
        self.conversion_callback = callback

    def setup_ui(self):
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # 历史记录面板
        history_frame = ttk.LabelFrame(main_frame, text="转换历史（最近10条）")
        history_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        self.history_text = scrolledtext.ScrolledText(
            history_frame,
            wrap=tk.WORD,
            font=("宋体", 10),
            height=15,
            width=35,
            padx=5,
            pady=5
        )
        self.history_text.pack(fill=tk.BOTH, expand=True)

        # 操作面板
        operation_frame = ttk.Frame(main_frame)
        operation_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        # 输入区域
        input_panel = ttk.Frame(operation_frame)
        input_panel.pack(fill=tk.X, pady=10)

        ttk.Label(input_panel, text="书名搜索:").pack(side=tk.LEFT)
        self.search_entry = ttk.Entry(input_panel)
        self.search_entry.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)
        self.search_entry.bind("<KeyRelease>", self.update_suggestions)

        self.convert_btn = ttk.Button(input_panel, text="转换", command=self.convert_book)
        self.convert_btn.pack(side=tk.LEFT)

        # 自动补全列表
        self.suggestion_list = tk.Listbox(
            operation_frame,
            height=8,
            selectbackground="#B3D9FF",
            exportselection=False
        )
        self.suggestion_list.bind("<<ListboxSelect>>", self.select_suggestion)

    def setup_context_menu(self):
        self.context_menu = Menu(self, tearoff=0)
        self.context_menu.add_command(label="复制", command=self.copy_history)
        self.history_text.bind("<Button-3>", self.show_context_menu)

    def update_suggestions(self, event=None):
        keyword = self.search_entry.get().strip()
        suggestions = self.get_sorted_suggestions(keyword)

        self.suggestion_list.delete(0, tk.END)
        for book in suggestions[:10]:
            self.suggestion_list.insert(tk.END, book)

        self.suggestion_list.configure(height=min(10, len(suggestions) or 1))
        self.suggestion_list.pack(fill=tk.X, pady=5)

    def get_sorted_suggestions(self, keyword):
        # 保持原有智能排序逻辑
        all_books = list(self.conversion_library.keys())
        if not keyword:
            return all_books

        scored = []
        for book in all_books:
            score = 0
            if book == keyword:
                score += 2000
            elif book.startswith(keyword):
                score += 1000
            elif keyword in book:
                score += 500 + (100 - book.find(keyword))
            score += (50 - len(book) // 2)
            if score > 0:
                scored.append((score, book))

        return [b for s, b in sorted(scored, reverse=True)]

    def select_suggestion(self, event):
        selection = self.suggestion_list.get(self.suggestion_list.curselection())
        self.search_entry.delete(0, tk.END)
        self.search_entry.insert(0, selection)
        self.suggestion_list.pack_forget()

    def convert_book(self):
        book_name = self.search_entry.get()
        code = self.conversion_library.get(book_name, None)

        if code:
            try:
                # 触发回调
                if self.conversion_callback:
                    self.conversion_callback(code)  # 新增回调触发
                # 转换成功逻辑
                self.add_history(f"{book_name} -> {code}")
            except ValueError:
                self.add_history(f"无效格式: {code}")
        else:
            self.add_history(f"未找到: {book_name}")

        self.search_entry.delete(0, tk.END)

    def add_history(self, record):
        self.history_text.configure(state=tk.NORMAL)
        self.history_text.insert(tk.END, record + "\n")

        # 保持最多10条记录
        line_count = int(self.history_text.index('end-1c').split('.')[0])
        while line_count > 10:
            self.history_text.delete("1.0", "2.0")
            line_count -= 1

        self.history_text.see(tk.END)
        self.history_text.configure(state=tk.DISABLED)

    def copy_history(self):
        try:
            selected = self.history_text.get(tk.SEL_FIRST, tk.SEL_LAST)
            self.clipboard_clear()
            self.clipboard_append(selected)
        except tk.TclError:
            pass

    def show_context_menu(self, event):
        self.context_menu.tk_popup(event.x_root, event.y_root)

    def set_send_callback(self, callback):
        """设置发送指令的回调函数"""
        self.send_command = callback
