# serial_assistant.py 串口助手模块
import tkinter as tk
from tkinter import ttk
from serial import Serial, SerialException
from serial.tools.list_ports import comports
from tkinter import messagebox
import re
import threading
import queue


class SerialAssistant(ttk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.master = master
        self.serial = None
        self.receive_mode = "HEX"
        self.send_mode = "HEX"
        self.encoding = "GBK"
        self.byte_buffer = bytearray()
        self.data_queue = queue.Queue()
        self.status_timer = None
        self.default_status = "等待操作"

        self.create_widgets()
        self.update_comports()
        self.after(100, self.process_data)
        self.create_status_bar()

    def create_widgets(self):
        # 串口配置区域
        config_frame = ttk.LabelFrame(self, text="串口配置")
        config_frame.pack(fill=tk.X, padx=5, pady=5)

        ttk.Label(config_frame, text="端口:").grid(row=0, column=0)
        self.port_combo = ttk.Combobox(config_frame, width=10)
        self.port_combo.grid(row=0, column=1, padx=5)

        self.refresh_btn = ttk.Button(config_frame, text="刷新", command=self.update_comports, width=6)
        self.refresh_btn.grid(row=0, column=2, padx=5)

        ttk.Label(config_frame, text="波特率:").grid(row=1, column=0)
        self.baud_combo = ttk.Combobox(config_frame, values=["4800", "9600", "19200", "38400", "115200"], width=10)
        self.baud_combo.current(1)
        self.baud_combo.grid(row=1, column=1)

        ttk.Label(config_frame, text="数据位:").grid(row=2, column=0)
        self.data_bits_combo = ttk.Combobox(config_frame, values=["5", "6", "7", "8"], width=10)
        self.data_bits_combo.current(3)
        self.data_bits_combo.grid(row=2, column=1)

        self.open_btn = ttk.Button(config_frame, text="打开串口", command=self.toggle_serial)
        self.open_btn.grid(row=3, column=0, columnspan=3, pady=5)

        # 主内容区域
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 接收区域
        receive_frame = ttk.LabelFrame(main_frame, text="接收区")
        receive_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        self.receive_text = tk.Text(receive_frame, height=15)
        self.receive_text.pack(fill=tk.BOTH, expand=True)
        ttk.Button(receive_frame, text="清空接收区", command=self.clear_receive).pack(pady=3)

        # 发送区域
        send_frame = ttk.LabelFrame(main_frame, text="发送区")
        send_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5)

        # 新增按钮面板
        button_frame = ttk.Frame(send_frame)
        button_frame.pack(fill=tk.BOTH, padx=5, pady=5)

        # 第一行按钮
        ttk.Button(button_frame, text="@CWRotate.",
                   command=lambda: self.send_direct_command("@CWRotate.")).grid(row=0, column=0, sticky="ew", padx=2,
                                                                                pady=2)
        ttk.Button(button_frame, text="@Forward.",
                   command=lambda: self.send_direct_command("@Forward.")).grid(row=0, column=1, sticky="ew", padx=2,
                                                                               pady=2)
        ttk.Button(button_frame, text="@CCWRotate.",
                   command=lambda: self.send_direct_command("@CCWRotate.")).grid(row=0, column=2, sticky="ew", padx=2,
                                                                                pady=2)

        # 第二行按钮
        ttk.Button(button_frame, text="@Left.",
                   command=lambda: self.send_direct_command("@Left.")).grid(row=1, column=0, sticky="ew", padx=2,
                                                                            pady=2)
        ttk.Button(button_frame, text="@Stop.",
                   command=lambda: self.send_direct_command("@Stop.")).grid(row=1, column=1, sticky="ew", padx=2,
                                                                            pady=2)
        ttk.Button(button_frame, text="@Right.",
                   command=lambda: self.send_direct_command("@Right.")).grid(row=1, column=2, sticky="ew", padx=2,
                                                                             pady=2)

        # 第三行按钮
        ttk.Button(button_frame, text="@F2_2_M3.",
                   command=lambda: self.send_direct_command("@F2_2_M3.")).grid(row=2, column=0, sticky="ew", padx=2,
                                                                                pady=2)
        ttk.Button(button_frame, text="@Backward.",
                   command=lambda: self.send_direct_command("@Backward.")).grid(row=2, column=1, sticky="ew", padx=2,
                                                                                pady=2)
        ttk.Button(button_frame, text="@Return.",
                   command=lambda: self.send_direct_command("@Return.")).grid(row=2, column=2, sticky="ew", padx=2,
                                                                              pady=2)

        # 设置按钮等宽
        for i in range(3):
            button_frame.columnconfigure(i, weight=1)
            button_frame.rowconfigure(i, weight=1)

        self.send_text = tk.Text(send_frame, height=5)
        self.send_text.pack(fill=tk.BOTH, expand=True)
        btn_frame = ttk.Frame(send_frame)
        btn_frame.pack(pady=3)
        ttk.Button(btn_frame, text="发送", command=self.send_data).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="清空发送区", command=self.clear_send).pack(side=tk.LEFT)

        # 配置选项
        option_frame = ttk.LabelFrame(self, text="配置选项")
        option_frame.pack(fill=tk.X, padx=5, pady=5)

        ttk.Label(option_frame, text="接收模式:").grid(row=0, column=0, padx=5)
        self.recv_mode_combo = ttk.Combobox(option_frame, values=["HEX", "文本"])
        self.recv_mode_combo.current(1)
        self.recv_mode_combo.grid(row=0, column=1, padx=5)

        ttk.Label(option_frame, text="发送模式:").grid(row=0, column=2, padx=5)
        self.send_mode_combo = ttk.Combobox(option_frame, values=["HEX", "文本"])
        self.send_mode_combo.current(1)
        self.send_mode_combo.grid(row=0, column=3, padx=5)

        ttk.Label(option_frame, text="编码方式:").grid(row=0, column=4, padx=5)
        self.encoding_combo = ttk.Combobox(option_frame, values=["GBK", "UTF-8"])
        self.encoding_combo.current(0)
        self.encoding_combo.grid(row=0, column=5, padx=5)

    def update_comports(self):
        current_selection = self.port_combo.get()
        ports = [port.device for port in comports()]
        self.port_combo["values"] = ports
        if ports:
            if current_selection in ports:
                self.port_combo.set(current_selection)
            else:
                self.port_combo.current(0)

    def send_direct_command(self, command):
        """直接发送按钮指令"""
        if not self.serial or not self.serial.is_open:
            self.set_status("错误：未连接串口", "red")
            return

        try:
            # 根据当前编码设置转换
            data = command.encode(self.encoding_combo.get())
            self.serial.write(data)
            self.set_status(f"指令发送成功: {command}", "green")
        except Exception as e:
            self.set_status(f"发送失败: {str(e)}", "red")

    def toggle_serial(self):
        if self.serial and self.serial.is_open:
            self.close_serial()
        else:
            self.open_serial()

    def open_serial(self):
        try:
            selected_port = self.port_combo.get()
            available_ports = [port.device for port in comports()]
            if selected_port not in available_ports:
                raise SerialException(f"端口 {selected_port} 不存在")

            self.serial = Serial(
                port=selected_port,
                baudrate=int(self.baud_combo.get()),
                bytesize=int(self.data_bits_combo.get()),
                parity="N",
                stopbits=1,
                timeout=1
            )
            self.open_btn.config(text="关闭串口")
            threading.Thread(target=self.read_serial, daemon=True).start()
        except Exception as e:
            messagebox.showerror("串口错误", f"打开串口失败:\n{str(e)}")
            self.close_serial()

    def close_serial(self):
        if self.serial:
            self.serial.close()
        self.open_btn.config(text="打开串口")

    def read_serial(self):
        while self.serial and self.serial.is_open:
            try:
                if self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)
                    self.data_queue.put(data)
            except SerialException:
                break

    def process_data(self):
        while not self.data_queue.empty():
            data = self.data_queue.get()
            if self.recv_mode_combo.get() == "HEX":
                text = " ".join([f"{b:02X}" for b in data])
            else:
                try:
                    text = data.decode(self.encoding_combo.get())
                except UnicodeDecodeError:
                    text = str(data)

            self.receive_text.insert(tk.END, text)
            self.receive_text.see(tk.END)
        self.after(100, self.process_data)

    def update_send_text(self, code):
        """更新发送区内容"""
        self.send_text.delete("1.0", tk.END)
        self.send_text.insert(tk.END, code)
        self.send_text.see(tk.END)  # 滚动到末尾

    def send_data(self):
        if not self.serial or not self.serial.is_open:
            self.set_status("错误：未连接串口", "red")
            return

        text = self.send_text.get("1.0", tk.END).strip()
        try:
            if self.send_mode_combo.get() == "HEX":
                if not text:
                    self.set_status("错误：HEX内容为空", "red")
                    return
                if len(text) % 2 != 0:
                    self.set_status("错误：HEX长度应为偶数", "red")
                    return
                text = re.sub(r"[^0-9A-Fa-f]", "", text)
                data = bytes.fromhex(text)
            else:
                if not text:
                    self.set_status("错误：文本内容为空", "red")
                    return
                data = text.encode(self.encoding_combo.get())

            self.serial.write(data)
            self.set_status("发送成功", "green")

        except ValueError as ve:
            self.set_status(f"HEX格式错误: {str(ve)}", "red")
        except SerialException as se:
            self.set_status(f"发送失败: {str(se)}", "red")
        except Exception as e:
            self.set_status(f"未知错误: {str(e)}", "red")

    def clear_receive(self):
        self.receive_text.delete("1.0", tk.END)

    def clear_send(self):
        self.send_text.delete("1.0", tk.END)

    def create_status_bar(self):
        status_frame = ttk.Frame(self)
        status_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        self.status_label = ttk.Label(status_frame, text=self.default_status, foreground="gray")
        self.status_label.pack(side=tk.RIGHT)

    def set_status(self, message, color="gray"):
        if self.status_timer:
            self.after_cancel(self.status_timer)
        self.status_label.config(text=message, foreground=color)
        self.status_timer = self.after(3000, self.restore_status)

    def restore_status(self):
        self.status_label.config(text=self.default_status, foreground="gray")
        self.status_timer = None
