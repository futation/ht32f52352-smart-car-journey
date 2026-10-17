# path_planner.py 路径规划模块
import tkinter as tk
from tkinter import ttk, messagebox, font
import matplotlib

matplotlib.use('TkAgg')
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import matplotlib.pyplot as plt
from matplotlib.widgets import TextBox, Button
import heapq
import pyperclip
import threading

# 设置全局中文字体
plt.rcParams['font.sans-serif'] = ['SimHei']  # 设置中文字体
plt.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题


class AStarPlanner(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.pack(fill=tk.BOTH, expand=True, side=tk.TOP)
        # 新增回调属性
        self.code_callback = None

        # 初始化Tkinter字体
        self.tk_font = font.Font(family="SimHei", size=10)
        self.option_add("*Font", self.tk_font)

        # 初始化图形
        self.fig = plt.figure(figsize=(14, 7))

        # 创建Tkinter组件
        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.toolbar = NavigationToolbar2Tk(self.canvas, self)
        self.toolbar.update()

        # 布局
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # 地图定义
        self.grid = [
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        ]

        # 坐标映射
        self.coord_map = {
            # 排数从下往上数，列数从左往右数

            # 第一排第二列
            '@01020A01.': (3, 13),
            '@01020A02.': (3, 15),
            '@01020A03.': (3, 17),
            '@01020A04.': (3, 19),
            '@01020A05.': (3, 21),
            # 第二排第二列
            '@02020A01.': (6, 13),
            '@02020A02.': (6, 15),
            '@02020A03.': (6, 17),
            '@02020A04.': (6, 19),
            '@02020A05.': (6, 21),
            # 第三排第二列
            '@03020A01.': (9, 13),
            '@03020A02.': (9, 15),
            '@03020A03.': (9, 17),
            '@03020A04.': (9, 19),
            '@03020A05.': (9, 21),

            # 第一排第一列
            '@01010A01.': (3, 9),
            '@01010A02.': (3, 7),
            '@01010A03.': (3, 5),
            '@01010A04.': (3, 3),
            '@01010A05.': (3, 1),
            # 第二排第一列
            '@02010A01.': (6, 9),
            '@02010A02.': (6, 7),
            '@02010A03.': (6, 5),
            '@02010A04.': (6, 3),
            '@02010A05.': (6, 1),
            # 第三排第一列
            '@03010A01.': (9, 9),
            '@03010A02.': (9, 7),
            '@03010A03.': (9, 5),
            '@03010A04.': (9, 3),
            '@03010A05.': (9, 1),
        }

        # 初始化界面组件
        self.init_ui()
        self.setup_map()
        self.setup_controls()

        # 增加右键菜单
        self.context_menu = tk.Menu(self, tearoff=0)
        self.context_menu.add_command(label="复制", command=self.copy_code)
        self.context_menu.add_command(label="粘贴", command=self.paste_to_code)

        # 绑定右键事件
        self.canvas.get_tk_widget().bind("<Button-3>", self.show_context_menu)

    def set_code_callback(self, callback):
        """设置编码更新回调"""
        self.code_callback = callback

    def init_ui(self):
        # 创建地图和控件区域
        self.map_ax = self.fig.add_axes([0.1, 0.1, 0.6, 0.8])
        self.control_ax = self.fig.add_axes([0.72, 0.3, 0.25, 0.4])
        self.control_ax.axis('off')

        # 初始化起点和路径
        self.start = (1, 11)
        self.goal = None
        self.path = None

        # 绑定事件
        self.fig.canvas.mpl_connect('button_press_event', self.on_map_click)
        self.fig.canvas.mpl_connect('key_press_event', self.on_key_press)


    def setup_map(self):
        """地图初始化"""
        self.map_ax.imshow(self.grid, cmap='binary_r', origin='lower',
                           extent=[0, len(self.grid[0]), 0, len(self.grid)])
        self.map_ax.set_xticks(range(len(self.grid[0])))
        self.map_ax.set_yticks(range(len(self.grid)))
        self.map_ax.grid(color='gray', linestyle='--', alpha=0.3)

        # 起点标记
        self.start_marker, = self.map_ax.plot(
            [self.start[1]], [self.start[0]],
            'gs', markersize=12, markeredgewidth=1,
            markeredgecolor='black', zorder=3
        )

        # 路径和终点标记
        self.path_line, = self.map_ax.plot([], [], 'r-', lw=1.5, zorder=2)
        self.goal_marker, = self.map_ax.plot(
            [], [], 'rs',
            markersize=12, markeredgewidth=1,
            markeredgecolor='black', zorder=3
        )

    def setup_controls(self):
        """控制面板布局"""
        # 编码输入框样式
        self.code_ax = self.fig.add_axes([0.72, 0.15, 0.25, 0.1])
        self.code_input = TextBox(
            self.code_ax, label='路径编码:',
            initial='@F0_0_M0.',
            textalignment='center',
            label_pad=0.01
        )
        # 设置输入框样式
        self.code_input.text_disp.set_color('#0044CC')  # 设置文字颜色
        self.code_input.label.set_color('darkred')  # 设置标签颜色
        self.code_ax.set_facecolor('#FFE4E1')  # 设置背景颜色
        for spine in self.code_ax.spines.values():  # 设置边框样式
            spine.set_color('darkred')
            spine.set_linewidth(1.5)

        # 添加输入框提示文本（右侧对齐）
        self.code_ax.text(0.95, 0.5, "右键复制",
                          ha='right', va='center',
                          fontsize=8, color='gray',
                          transform=self.code_ax.transAxes)

        # 坐标输入框样式
        self.text_ax = self.fig.add_axes([0.75, 0.6, 0.2, 0.05])
        self.coord_input = TextBox(
            self.text_ax,
            label='',
            textalignment='center',
            label_pad=0.1
        )
        self.coord_input.text_disp.set_color('gray')

        # 示例文本
        self.example_ax = self.fig.add_axes([0.72, 0.7, 0.25, 0.2])
        self.example_ax.axis('off')
        self.example_ax.text(
            0.12, 0.1,
            "输入示例:\n@01010A01.\n点击地图也可设置目标,也可直接输入坐标",
            fontsize=10,
            verticalalignment='top'
        )

        # 在坐标输入框添加提示文本（新增）
        self.text_ax.text(0.95, 0.5, "右键粘贴",
                          ha='right', va='center',
                          fontsize=8, color='gray',
                          transform=self.text_ax.transAxes)

        # 设置坐标输入框样式（新增）
        self.text_ax.set_facecolor('#F0F0F0')
        for spine in self.text_ax.spines.values():
            spine.set_color('blue')
            spine.set_linewidth(1)

        # 确定按钮
        self.btn_ax = self.fig.add_axes([0.75, 0.5, 0.1, 0.05])
        self.confirm_btn = Button(self.btn_ax, '确定')

        # 错误提示（调整到按钮正下方）
        self.error_ax = self.fig.add_axes([0.72, 0.35, 0.25, 0.08])  # Y坐标下移
        self.error_ax.axis('off')
        self.error_text = self.error_ax.text(
            0.05, 0.5, "",
            fontsize=10,
            color='red',
            visible=False,
            wrap=True  # 添加自动换行
        )
        # 在错误提示代码段添加以下样式设置
        self.error_ax.set_facecolor('#FFE4E1')  # 浅红色背景
        self.error_ax.set_zorder(100)  # 确保在最上层
        # 在error_ax周围添加边框
        for spine in self.error_ax.spines.values():
            spine.set_color('red')
            spine.set_linewidth(1)

        # 事件绑定
        self.confirm_btn.on_clicked(self.on_submit)
        self.coord_input.on_submit(self.on_submit)

        # 在创建所有axes后添加层级调整
        self.error_ax.set_zorder(100)  # 错误提示置顶
        self.code_ax.set_zorder(99)
        self.text_ax.set_zorder(98)
        self.btn_ax.set_zorder(97)

    def on_map_click(self, event):
        """地图点击事件处理"""
        if event.inaxes != self.map_ax:
            return

        # 转换点击坐标为行列索引
        col = int(round(event.xdata))
        row = int(round(event.ydata))
        self.set_goal((row, col))

    def set_goal(self, goal):
        """增强型目标设置（添加坐标转换）"""
        try:
            # 新增：如果是字符串编码，优先从映射表查找
            if isinstance(goal, str) and goal in self.coord_map:
                goal = self.coord_map[goal]
            # 转换不同坐标格式
            if isinstance(goal, str):
                if goal in self.coord_map:
                    goal = self.coord_map[goal]
                else:
                    goal = tuple(map(int, goal.strip('()').split(',')))

            # 有效性验证（保持你原有的调用方式）
            if not (0 <= goal[0] < len(self.grid)):
                self.show_error(f"行号越界 (0-{len(self.grid) - 1})")  # 自动填充title为None
                return
            if not (0 <= goal[1] < len(self.grid[0])):
                self.show_error(f"列号越界 (0-{len(self.grid[0]) - 1})")
                return
            if self.grid[goal[0]][goal[1]] != 0:
                self.show_error("目标点不可位于障碍物上")
                return

            self.goal = goal
            threading.Thread(target=self.run_astar, daemon=True).start()

        except Exception as e:
            self.after(0, lambda: self.show_error(str(e)))

    def run_astar(self):
        """执行A*算法并更新界面"""
        self.path = self.astar_search()
        self.after(0, self.update_display)

    # 增强update_display方法
    def update_display(self):
        """更新界面显示"""
        try:
            code = self.generate_path_code() if self.path else "@F0_0_M0."
            self.code_input.set_val(code)

            # 触发回调
            if self.code_callback:
                self.code_callback(code)

            # 更新终点标记
            if self.goal:
                self.goal_marker.set_data([self.goal[1]], [self.goal[0]])
                self.goal_marker.set_visible(True)
            else:
                self.goal_marker.set_visible(False)

            # 更新路径线
            if self.path:
                x, y = zip(*self.path)
                self.path_line.set_data(y, x)
                self.path_line.set_visible(True)
            else:
                self.path_line.set_visible(False)

            # 强制刷新画布
            self.canvas.draw_idle()

        except Exception as e:
            self.after(0, lambda: self.show_error("显示更新失败"))

    # 保留A*算法相关方法（与原始代码相同）
    class Node:
        def __init__(self, position, g=0, h=0, parent=None):
            self.position = position
            self.g = g
            self.h = h
            self.f = g + h
            self.parent = parent

        def __lt__(self, other):
            return self.f < other.f

    def heuristic(self, a, b):
        return abs(a[0] - b[0]) + abs(a[1] - b[1])

    def get_neighbors(self, current):
        directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]
        neighbors = []
        rows, cols = len(self.grid), len(self.grid[0])
        for dx, dy in directions:
            x = current[0] + dx
            y = current[1] + dy
            if 0 <= x < rows and 0 <= y < cols and self.grid[x][y] == 0:
                neighbors.append((x, y))
        return neighbors

    def astar_search(self):
        if not self.goal: return

        open_heap = []
        closed = set()
        node_cache = {}

        start_node = self.Node(self.start, 0, self.heuristic(self.start, self.goal))
        heapq.heappush(open_heap, (start_node.f, start_node))
        node_cache[self.start] = start_node

        while open_heap:
            current_f, current_node = heapq.heappop(open_heap)

            if current_node.position == self.goal:
                path = []
                tmp = current_node
                while tmp:
                    path.append(tmp.position)
                    tmp = tmp.parent
                return path[::-1]

            closed.add(current_node.position)

            for neighbor in self.get_neighbors(current_node.position):
                if neighbor in closed:
                    continue

                tentative_g = current_node.g + 1

                if neighbor in node_cache:
                    existing = node_cache[neighbor]
                    if tentative_g < existing.g:
                        existing.g = tentative_g
                        existing.f = tentative_g + existing.h
                        existing.parent = current_node
                        heapq.heapify(open_heap)
                else:
                    new_node = self.Node(
                        neighbor,
                        tentative_g,
                        self.heuristic(neighbor, self.goal),
                        current_node
                    )
                    node_cache[neighbor] = new_node
                    heapq.heappush(open_heap, (new_node.f, new_node))
        return None

    def generate_path_code(self):
        """生成路径编码"""
        if not self.path or len(self.path) < 2:
            return "@F0_0_M0."

        start = self.path[0]
        end = self.path[-1]

        x = abs(end[0] - start[0])
        z_distance = abs(end[1] - start[1])
        y_direction = 0 if z_distance == 0 else (1 if end[1] < start[1] else 2)

        return f"@F{x}_{y_direction}_M{z_distance}."

    def on_submit(self, event=None):
        """增强型表单提交"""
        try:
            input_text = self.coord_input.text_disp.get_text().strip()
            if not input_text:
                return

            # 自动识别输入类型
            if input_text.startswith('@'):
                self.handle_code_input(input_text)
            elif '(' in input_text or ',' in input_text:
                self.handle_coord_input(input_text)
            else:
                raise ValueError("无法识别的输入格式")

            # 清空输入框
            self.after(0, lambda: self.coord_input.set_val(""))

        except Exception as e:
            self.after(0, self.show_error("输入错误", str(e)))

    def on_key_press(self, event):
        """增强键盘事件处理"""
        # 编码输入框处理
        if event.inaxes == self.code_ax:
            if event.key.lower() == 'ctrl+c':
                self.copy_code()
                return
            elif event.key.lower() == 'ctrl+v':
                self.paste_to_code()
                return

        # 坐标输入框处理（新增部分）
        if event.inaxes == self.text_ax:
            if event.key.lower() == 'ctrl+c':
                self.copy_coord()
                return
            elif event.key.lower() == 'ctrl+v':
                self.paste_to_coord()
                return

        # if event.inaxes == self.code_ax:
        #     # 处理组合键
        #     if event.key.lower() == 'ctrl+c':
        #         try:
        #             pyperclip.copy(self.code_input.text_disp.get_text())
        #             self.show_temp_message("已复制!", 'green')
        #             return
        #         except Exception as e:
        #             self.show_error("错误", f"复制失败: {str(e)}")
        #
        #     elif event.key.lower() == 'ctrl+v':
        #         try:
        #             clipboard_content = pyperclip.paste()
        #             if clipboard_content:
        #                 self.code_input.set_val(clipboard_content)
        #                 self.fig.canvas.draw()
        #                 return
        #         except Exception as e:
        #             self.show_error("错误", f"粘贴失败: {str(e)}")

            # 允许常规文本输入
            self.code_input.set_active(True)

    def show_context_menu(self, event):
        """增强右键菜单显示逻辑"""
        # 转换点击坐标到归一化坐标
        fig_width, fig_height = self.fig.get_size_inches()
        x_norm = event.x / (fig_width * self.fig.dpi)
        y_norm = 1 - event.y / (fig_height * self.fig.dpi)  # Matplotlib坐标系反向

        # 判断点击区域（新增坐标输入框判断）
        if self._is_in_axes(self.code_ax, x_norm, y_norm):
            self.context_menu.entryconfig("复制", command=self.copy_code)
            self.context_menu.entryconfig("粘贴", command=self.paste_to_code)
            self.context_menu.post(event.x_root, event.y_root)
        elif self._is_in_axes(self.text_ax, x_norm, y_norm):  # 新增坐标框判断
            self.context_menu.entryconfig("复制", command=self.copy_coord)
            self.context_menu.entryconfig("粘贴", command=self.paste_to_coord)
            self.context_menu.post(event.x_root, event.y_root)

    def copy_code(self):
        """增强复制功能"""
        try:
            current_text = self.code_input.text_disp.get_text()
            if current_text:
                pyperclip.copy(current_text)
                self.show_temp_message("已复制!", 'green')
            else:
                self.show_temp_message("内容为空", 'orange')
        except Exception as e:
            self.show_error("错误", f"复制失败: {str(e)}")

    def paste_to_code(self):
        """增强粘贴功能"""
        try:
            clipboard_content = pyperclip.paste()
            if clipboard_content:
                # 保留原有内容并追加（可根据需要修改为覆盖模式）
                current = self.code_input.text_disp.get_text()
                self.code_input.set_val(current + clipboard_content)
                self.fig.canvas.draw()
        except Exception as e:
            self.show_error("错误", f"粘贴失败: {str(e)}")

    def show_temp_message(self, text, color):
        """优化临时提示显示"""
        self.code_ax.set_title(text, color=color, fontsize=10, pad=5)
        self.fig.canvas.draw()
        self.after(1000, lambda: self.code_ax.set_title(""))

    def handle_coord_input(self, text):
        """处理坐标输入"""
        try:
            clean_text = text.strip('()[]{}')
            x, y = map(int, clean_text.split(','))
            self.set_goal((x, y))
        except Exception as e:
            self.after(0, lambda: self.show_error("坐标格式错误",f"无效坐标格式: {text}\n正确格式示例: (3,5) 或 3,5"))


    def handle_code_input(self, code):
        """处理编码输入"""
        try:
            if code not in self.coord_map:
                raise ValueError(f"未知编码: {code}")
            self.set_goal(self.coord_map[code])
        except Exception as e:
            self.after(0, lambda: self.show_error("编码错误", str(e)))

    # 修改错误显示方法
    def show_error(self, message, title=None):  # 添加title为可选参数
        """显示错误信息（优化版）"""
        try:
            # 自动换行处理（兼容中文）
            wrapped_lines = []
            current_line = ""
            for char in message:
                if len(current_line) >= 15:  # 按字符数计算
                    wrapped_lines.append(current_line)
                    current_line = ""
                current_line += char
            if current_line:
                wrapped_lines.append(current_line)
            wrapped_text = '\n'.join(wrapped_lines)

            # 添加标题显示（如果存在）
            if title:
                wrapped_text = f"[{title}]\n" + wrapped_text

            # 清空原有内容并设置新文本
            self.error_text.set_text(wrapped_text)
            self.error_text.set_position((0.05, 0.3))  # 调整到顶部
            self.error_text.set_verticalalignment('top')
            self.error_text.set_color('red')
            self.error_text.set_verticalalignment('bottom')

            # 调整错误框高度
            line_count = len(wrapped_lines)
            if title: line_count += 1  # 标题占一行
            self.error_ax.set_position([0.72, 0.35, 0.25, 0.08 + 0.05 * line_count])

            # 强制刷新显示
            self.error_text.set_visible(True)
            self.canvas.draw_idle()  # 刷新整个画布

            # 设置定时隐藏
            if hasattr(self, "_error_timer"):
                self.after_cancel(self._error_timer)
            self._error_timer = self.after(5000, self.hide_error)
        except Exception as e:
            print(f"Error display failed: {str(e)}")

    def hide_error(self):
        """隐藏错误信息"""
        self.error_text.set_visible(False)
        self.fig.canvas.draw()

    def copy_coord(self):
        """复制坐标输入框内容"""
        try:
            current_text = self.coord_input.text_disp.get_text()
            if current_text:
                pyperclip.copy(current_text)
                self.show_temp_message("已复制坐标!", 'green')
            else:
                self.show_temp_message("坐标内容为空", 'orange')
        except Exception as e:
            self.show_error("错误", f"复制失败: {str(e)}")


    def paste_to_coord(self):
        """粘贴到坐标输入框"""
        try:
            clipboard_content = pyperclip.paste()
            if clipboard_content:
                # 保留原有内容（可根据需要改为覆盖）
                current = self.coord_input.text_disp.get_text()
                self.coord_input.set_val(current + clipboard_content)
                self.fig.canvas.draw()
        except Exception as e:
            self.show_error("错误", f"粘贴失败: {str(e)}")

    def _is_in_axes(self, ax, x_norm, y_norm):
        """判断归一化坐标是否在指定axes内"""
        pos = ax.get_position()
        return (pos.x0 < x_norm < pos.x1 and
                pos.y0 < y_norm < pos.y1)

    def set_coordinate(self, code):
        """设置坐标输入框内容"""

        def update_ui():
            try:
                # 设置输入框内容
                self.coord_input.set_val(code)
                # 触发路径规划
                self.set_goal(code)
                # 视觉反馈
                self.coord_input.text_disp.set_color('blue')
                self.fig.canvas.draw_idle()
                self.after(1000, lambda: self.coord_input.text_disp.set_color('black'))
            except Exception as e:
                print(f"更新坐标时发生错误: {str(e)}")

        if self.winfo_exists():
            self.after(0, update_ui)