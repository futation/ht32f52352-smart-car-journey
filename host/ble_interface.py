# ble_interface.py
#蓝牙模块
import asyncio
import tkinter as tk
from tkinter import ttk, messagebox
from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError
from threading import Thread


class BleakController:
    # 保持原有BleakController实现不变
    def __init__(self, receive_callback=None):
        self.client = None
        self.loop = asyncio.new_event_loop()
        self.service_uuid = "0000ffe0-0000-1000-8000-00805f9b34fb"
        self.char_uuid = "0000ffe1-0000-1000-8000-00805f9b34fb"
        self.receive_callback = receive_callback

    async def scan_devices(self):
        devices = await BleakScanner.discover()
        return [{"name": d.name, "address": d.address} for d in devices if d.name is not None]

    async def connect_device(self, address):
        self.client = BleakClient(address, loop=self.loop)
        await self.client.connect()
        await self.client.start_notify(self.char_uuid, self._notification_handler)
        return self.client.is_connected

    async def send_data(self, data):
        if not self.client.is_connected:
            raise BleakError("Device not connected")
        await self.client.write_gatt_char(self.char_uuid, data)

    def _notification_handler(self, sender, data):
        if self.receive_callback:
            self.receive_callback(data)


class BLEApp(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.ble = BleakController(receive_callback=self.handle_received_data)
        self.create_widgets()
        self.setup_thread()

    def setup_thread(self):
        self.thread = Thread(target=self.run_async)
        self.thread.daemon = True
        self.thread.start()

    def run_async(self):
        asyncio.set_event_loop(self.ble.loop)
        self.ble.loop.run_forever()

    def create_widgets(self):
        # 蓝牙设备扫描区域
        self.scan_frame = ttk.LabelFrame(self, text="蓝牙设备管理")
        self.scan_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 扫描按钮
        self.scan_btn = ttk.Button(self.scan_frame, text="扫描设备", command=self.start_scan)
        self.scan_btn.pack(pady=5)

        # 设备列表
        self.device_tree = ttk.Treeview(self.scan_frame, columns=("name", "address"), show="headings", height=8)
        self.device_tree.heading("name", text="设备名称")
        self.device_tree.heading("address", text="MAC地址")
        self.device_tree.pack(fill=tk.BOTH, expand=True)

        # 状态栏
        self.status_bar = ttk.Label(self.scan_frame, text="就绪", foreground="gray")
        self.status_bar.pack(pady=3)

        # 连接按钮
        self.connect_btn = ttk.Button(self.scan_frame, text="连接设备", command=self.connect_selected_device)
        self.connect_btn.pack(pady=5)

    def start_scan(self):
        self.status_bar.config(text="扫描中...", foreground="blue")
        self.scan_btn.config(state=tk.DISABLED)

        async def _scan():
            try:
                devices = await self.ble.scan_devices()
                self.device_tree.delete(*self.device_tree.get_children())
                for d in devices:
                    self.device_tree.insert("", tk.END, values=(d["name"], d["address"]))
            except Exception as e:
                self.after(0, messagebox.showerror, "扫描错误", str(e))
            finally:
                self.after(0, lambda: self.status_bar.config(text="搜索完成", foreground="green"))
                self.after(0, lambda: self.scan_btn.config(state=tk.NORMAL))

        asyncio.run_coroutine_threadsafe(_scan(), self.ble.loop)

    def connect_selected_device(self):
        selected_item = self.device_tree.selection()
        if selected_item:
            values = self.device_tree.item(selected_item, 'values')
            address = values[1]

            async def _connect():
                try:
                    connected = await self.ble.connect_device(address)
                    if connected:
                        messagebox.showinfo("成功", "设备连接成功")
                    else:
                        messagebox.showerror("错误", "设备连接失败")
                except Exception as e:
                    messagebox.showerror("错误", str(e))

            asyncio.run_coroutine_threadsafe(_connect(), self.ble.loop)
        else:
            messagebox.showerror("错误", "请选择一个设备进行连接")

    def handle_received_data(self, data):
        def update_ui():
            hex_str = ' '.join(f"{b:02X}" for b in data)
            self.status_bar.config(text=f"最后接收: {hex_str}", foreground="black")

        self.after(0, update_ui)

    def send_command(self, command):
        async def _send():
            await self.ble.send_data(command.encode())

        asyncio.run_coroutine_threadsafe(_send(), self.ble.loop)
