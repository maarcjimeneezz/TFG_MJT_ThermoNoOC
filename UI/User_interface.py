import customtkinter as ctk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from matplotlib.animation import FuncAnimation
import threading
import json
from datetime import datetime
from collections import deque
import socket

# Set appearance mode
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")


class SensorData:
    """Store and manage sensor data"""
    def __init__(self, max_points=500):
        self.max_points = max_points
        self.timestamps = deque(maxlen=max_points)
        self.temp1 = deque(maxlen=max_points)
        self.humidity1 = deque(maxlen=max_points)
        self.temp2 = deque(maxlen=max_points)
        self.humidity2 = deque(maxlen=max_points)
        self.uv = deque(maxlen=max_points)
        self.co2 = deque(maxlen=max_points)
        self.flow1 = deque(maxlen=max_points)
        self.flow2 = deque(maxlen=max_points)
        
    def add_data(self, data_dict):
        """Add data point to sensor history"""
        self.timestamps.append(data_dict.get('timestamp', datetime.now()))
        self.temp1.append(data_dict.get('temp1', 0))
        self.humidity1.append(data_dict.get('humidity1', 0))
        self.temp2.append(data_dict.get('temp2', 0))
        self.humidity2.append(data_dict.get('humidity2', 0))
        self.uv.append(data_dict.get('uv', 0))
        self.co2.append(data_dict.get('co2', 0))
        self.flow1.append(data_dict.get('flow1', 0))
        self.flow2.append(data_dict.get('flow2', 0))


class IncubatorUI(ctk.CTk):
    """Main application interface for incubator control"""
    
    def __init__(self):
        super().__init__()
        
        # Window configuration
        self.title("ThermoNoOC Incubator Control System")
        self.geometry("1600x1000")
        self.minsize(1400, 900)
        
        # Data storage
        self.sensor_data = SensorData()
        self.wifi_connected = False
        self.esp32_host = "192.168.4.1"
        self.esp32_port = 80
        
        # Setup UI
        self.setup_ui()
        self.start_data_simulation()
        
    def setup_ui(self):
        """Setup main UI layout"""
        # Configure grid
        self.grid_rowconfigure(0, weight=0)
        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)
        
        # Header
        self.create_header()
        
        # Main content frame with scrolling
        self.main_frame = ctk.CTkFrame(self)
        self.main_frame.grid(row=1, column=0, sticky="nsew", padx=10, pady=10)
        self.main_frame.grid_columnconfigure(0, weight=1)
        self.main_frame.grid_rowconfigure(0, weight=1)
        
        # Create scrollable frame
        self.scrollable_frame = ctk.CTkScrollableFrame(self.main_frame)
        self.scrollable_frame.grid(row=0, column=0, sticky="nsew")
        self.scrollable_frame.grid_columnconfigure(0, weight=1)
        
        # Content sections
        self.create_sensor_monitoring_section()
        self.create_temperature_control_section()
        self.create_uv_control_section()
        self.create_microfluidics_section()
        
    def create_header(self):
        """Create top header with connection status"""
        header_frame = ctk.CTkFrame(self, height=60)
        header_frame.grid(row=0, column=0, sticky="ew", padx=10, pady=10)
        header_frame.grid_columnconfigure(0, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            header_frame,
            text="ThermoNoOC Incubator Control System",
            font=("Helvetica", 24, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=20)
        
        # Connection status
        self.connection_indicator = ctk.CTkLabel(
            header_frame,
            text="●",
            font=("Helvetica", 16),
            text_color="#ff4444"
        )
        self.connection_indicator.grid(row=0, column=1, padx=20)
        
        self.connection_label = ctk.CTkLabel(
            header_frame,
            text="Disconnected",
            font=("Helvetica", 12)
        )
        self.connection_label.grid(row=0, column=2, sticky="e", padx=20)
        
        # Connect button
        self.connect_btn = ctk.CTkButton(
            header_frame,
            text="Connect ESP32",
            command=self.connect_to_esp32,
            width=150,
            height=40
        )
        self.connect_btn.grid(row=0, column=3, padx=10)
        
    def create_sensor_monitoring_section(self):
        """Create incubator sensor monitoring section with graphs"""
        section_frame = ctk.CTkFrame(self.scrollable_frame)
        section_frame.grid(row=0, column=0, sticky="ew", padx=5, pady=10)
        section_frame.grid_columnconfigure(0, weight=1)
        
        # Title with save button
        header_frame = ctk.CTkFrame(section_frame)
        header_frame.grid(row=0, column=0, sticky="ew", padx=15, pady=10)
        header_frame.grid_columnconfigure(0, weight=1)
        
        title_label = ctk.CTkLabel(
            header_frame,
            text="🌡️ Incubator Monitoring",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w")
        
        self.save_data_btn = ctk.CTkButton(
            header_frame,
            text="💾 Save Data to CSV",
            command=self.save_data_to_csv,
            width=150,
            height=30
        )
        self.save_data_btn.grid(row=0, column=1, sticky="e")
        
        # Create graph frame
        graph_container = ctk.CTkFrame(section_frame)
        graph_container.grid(row=1, column=0, sticky="nsew", padx=10, pady=10)
        graph_container.grid_columnconfigure((0, 1), weight=1)
        graph_container.grid_rowconfigure((0, 1), weight=1)
        
        # Combined Temperature and Humidity Graph (both sensors)
        self.fig_temp = Figure(figsize=(7, 3.5), dpi=100)
        self.ax_temp = self.fig_temp.add_subplot(121)
        self.ax_humidity = self.fig_temp.add_subplot(122)
        self.fig_temp.patch.set_facecolor('#212121')
        
        canvas_temp = FigureCanvasTkAgg(self.fig_temp, graph_container)
        canvas_temp.get_tk_widget().grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        
        # UV and CO2 Graph
        self.fig_env = Figure(figsize=(7, 3.5), dpi=100)
        self.ax_uv = self.fig_env.add_subplot(121)
        self.ax_co2 = self.fig_env.add_subplot(122)
        self.fig_env.patch.set_facecolor('#212121')
        
        canvas_env = FigureCanvasTkAgg(self.fig_env, graph_container)
        canvas_env.get_tk_widget().grid(row=0, column=1, sticky="nsew", padx=5, pady=5)
        
        # Current values display
        values_frame = ctk.CTkFrame(section_frame)
        values_frame.grid(row=2, column=0, sticky="ew", padx=10, pady=10)
        values_frame.grid_columnconfigure((0, 1, 2, 3), weight=1)
        
        # Sensor value labels
        self.labels = {}
        sensors = [
            ('temp1', 'Temp 1', '°C'),
            ('humidity1', 'Humidity 1', '%'),
            ('temp2', 'Temp 2', '°C'),
            ('humidity2', 'Humidity 2', '%'),
            ('uv', 'UV', 'W/m²'),
            ('co2', 'CO₂', 'ppm')
        ]
        
        for idx, (key, label, unit) in enumerate(sensors):
            frame = ctk.CTkFrame(values_frame)
            frame.grid(row=0, column=idx, sticky="nsew", padx=5)
            frame.grid_rowconfigure((0, 1), weight=1)
            
            lbl = ctk.CTkLabel(frame, text=label, font=("Helvetica", 10, "bold"))
            lbl.grid(row=0, column=0)
            
            value_lbl = ctk.CTkLabel(
                frame,
                text="0.0",
                font=("Helvetica", 14, "bold"),
                text_color="#00ff00"
            )
            value_lbl.grid(row=1, column=0)
            
            unit_lbl = ctk.CTkLabel(frame, text=unit, font=("Helvetica", 9))
            unit_lbl.grid(row=2, column=0)
            
            self.labels[key] = value_lbl
        
        self.setup_graph_updates()
        
    def create_temperature_control_section(self):
        """Create temperature control section"""
        section_frame = ctk.CTkFrame(self.scrollable_frame)
        section_frame.grid(row=1, column=0, sticky="ew", padx=5, pady=10)
        section_frame.grid_columnconfigure(0, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            section_frame,
            text="🌡️ Temperature Control",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=15, pady=10)
        
        # Control frame
        control_frame = ctk.CTkFrame(section_frame)
        control_frame.grid(row=1, column=0, sticky="ew", padx=15, pady=15)
        control_frame.grid_columnconfigure(1, weight=1)
        
        # Slider
        label = ctk.CTkLabel(control_frame, text="Target Temperature:", font=("Helvetica", 12, "bold"))
        label.grid(row=0, column=0, sticky="w", padx=10, pady=10)
        
        self.temp_slider = ctk.CTkSlider(
            control_frame,
            from_=20,
            to=50,
            number_of_steps=150,
            command=self.update_target_temp,
            height=8
        )
        self.temp_slider.set(37)
        self.temp_slider.grid(row=0, column=1, sticky="ew", padx=20, pady=10)
        
        self.temp_value_label = ctk.CTkLabel(
            control_frame,
            text="37.0°C",
            font=("Helvetica", 14, "bold"),
            text_color="#00ff00",
            width=80
        )
        self.temp_value_label.grid(row=0, column=2, padx=10, pady=10)
        
        # Entry field for manual input
        entry_label = ctk.CTkLabel(control_frame, text="Or enter value:", font=("Helvetica", 11))
        entry_label.grid(row=1, column=0, sticky="w", padx=10, pady=10)
        
        self.temp_entry = ctk.CTkEntry(
            control_frame,
            placeholder_text="Enter temperature (20-50)",
            width=150
        )
        self.temp_entry.grid(row=1, column=1, sticky="w", padx=20, pady=10)
        
        set_btn = ctk.CTkButton(
            control_frame,
            text="Set",
            command=self.set_temp_from_entry,
            width=80
        )
        set_btn.grid(row=1, column=2, padx=10, pady=10)
        
    def create_uv_control_section(self):
        """Create UV LED control section"""
        section_frame = ctk.CTkFrame(self.scrollable_frame)
        section_frame.grid(row=2, column=0, sticky="ew", padx=5, pady=10)
        section_frame.grid_columnconfigure(0, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            section_frame,
            text="💡 UV LED Control (4 Groups)",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=15, pady=10)
        
        # LED groups frame
        leds_frame = ctk.CTkFrame(section_frame)
        leds_frame.grid(row=1, column=0, sticky="ew", padx=15, pady=15)
        leds_frame.grid_columnconfigure((0, 1, 2, 3), weight=1)
        
        self.uv_sliders = {}
        self.uv_labels = {}
        self.uv_switches = {}
        
        for i in range(1, 5):
            # Frame for each LED group
            frame = ctk.CTkFrame(leds_frame)
            frame.grid(row=0, column=i-1, sticky="nsew", padx=10, pady=10)
            frame.grid_rowconfigure((0, 1, 2, 3, 4), weight=1)
            
            # Title
            group_label = ctk.CTkLabel(
                frame,
                text=f"UV Group {i}",
                font=("Helvetica", 12, "bold")
            )
            group_label.grid(row=0, column=0, sticky="ew", pady=5)
            
            # Enable/Disable toggle
            switch = ctk.CTkSwitch(
                frame,
                text="Enabled",
                command=lambda idx=i: self.toggle_uv_group(idx)
            )
            switch.grid(row=1, column=0, sticky="ew", pady=5)
            self.uv_switches[i] = switch
            
            # Intensity slider
            slider_label = ctk.CTkLabel(frame, text="Intensity", font=("Helvetica", 10))
            slider_label.grid(row=2, column=0, sticky="w", pady=5)
            
            slider = ctk.CTkSlider(
                frame,
                from_=0,
                to=100,
                number_of_steps=100,
                command=lambda val, idx=i: self.update_uv_intensity(idx, val),
                height=6
            )
            slider.set(50)
            slider.grid(row=3, column=0, sticky="ew", pady=5)
            self.uv_sliders[i] = slider
            
            # Value display
            value_label = ctk.CTkLabel(
                frame,
                text="50%",
                font=("Helvetica", 12, "bold"),
                text_color="#ffaa00"
            )
            value_label.grid(row=4, column=0, sticky="ew", pady=5)
            self.uv_labels[i] = value_label
        
    def create_microfluidics_section(self):
        """Create microfluidics control section"""
        section_frame = ctk.CTkFrame(self.scrollable_frame)
        section_frame.grid(row=3, column=0, sticky="ew", padx=5, pady=10)
        section_frame.grid_columnconfigure(0, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            section_frame,
            text="💧 Microfluidics Section",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=15, pady=10)
        
        # Flow sensors graph
        self.fig_flow = Figure(figsize=(14, 3), dpi=100)
        self.ax_flow = self.fig_flow.add_subplot(111)
        self.fig_flow.patch.set_facecolor('#212121')
        
        canvas_flow = FigureCanvasTkAgg(self.fig_flow, section_frame)
        canvas_flow.get_tk_widget().grid(row=1, column=0, sticky="nsew", padx=10, pady=10)
        
        # Pump controls frame
        pumps_frame = ctk.CTkFrame(section_frame)
        pumps_frame.grid(row=2, column=0, sticky="ew", padx=15, pady=15)
        pumps_frame.grid_columnconfigure((0, 1), weight=1)
        
        self.pump_entries = {}
        self.pump_labels = {}
        
        for pump_id in range(1, 3):
            # Frame for each pump
            frame = ctk.CTkFrame(pumps_frame)
            frame.grid(row=0, column=pump_id-1, sticky="nsew", padx=10, pady=10)
            frame.grid_columnconfigure(1, weight=1)
            
            # Title
            pump_label = ctk.CTkLabel(
                frame,
                text=f"Micropump {pump_id}",
                font=("Helvetica", 12, "bold")
            )
            pump_label.grid(row=0, column=0, columnspan=2, sticky="w", pady=5)
            
            # Flow target
            flow_label = ctk.CTkLabel(frame, text="Target Flow (µL/min):", font=("Helvetica", 10))
            flow_label.grid(row=1, column=0, sticky="w", pady=5)
            
            entry = ctk.CTkEntry(
                frame,
                placeholder_text="0-100",
                width=120
            )
            entry.insert(0, "0")
            entry.grid(row=1, column=1, sticky="w", padx=10, pady=5)
            self.pump_entries[pump_id] = entry
            
            # Current flow display
            current_label = ctk.CTkLabel(
                frame,
                text="Current Flow: 0.0 µL/min",
                font=("Helvetica", 10),
                text_color="#00ff00"
            )
            current_label.grid(row=2, column=0, columnspan=2, sticky="w", pady=5)
            self.pump_labels[pump_id] = current_label
            
            # Set button
            set_btn = ctk.CTkButton(
                frame,
                text="Set Flow",
                command=lambda idx=pump_id: self.set_pump_flow(idx),
                width=100
            )
            set_btn.grid(row=3, column=0, columnspan=2, sticky="ew", pady=10)
        
    def setup_graph_updates(self):
        """Setup graph animation"""
        def update_graphs():
            while True:
                self.update_temperature_graph()
                self.update_env_graph()
                self.update_flow_graph()
                self.update_sensor_labels()
                self.after(1000)
        
        thread = threading.Thread(target=update_graphs, daemon=True)
        thread.start()
        
    def update_temperature_graph(self):
        """Update combined temperature and humidity graphs from both sensors"""
        self.ax_temp.clear()
        self.ax_humidity.clear()
        
        if len(self.sensor_data.temp1) > 0:
            x = range(len(self.sensor_data.temp1))
            
            # Combined Temperature (both sensors)
            self.ax_temp.plot(x, list(self.sensor_data.temp1), label='Sensor 1', color='#ff6b6b', linewidth=2)
            self.ax_temp.plot(x, list(self.sensor_data.temp2), label='Sensor 2', color='#ff9999', linewidth=2, linestyle='--')
            self.ax_temp.set_title('Temperature (Both Sensors)', fontsize=11, color='white')
            self.ax_temp.set_ylabel('Temperature (°C)', color='white')
            self.ax_temp.tick_params(colors='white')
            self.ax_temp.legend(loc='upper left', fontsize=9)
            self.ax_temp.grid(True, alpha=0.2)
            
            # Combined Humidity (both sensors)
            self.ax_humidity.plot(x, list(self.sensor_data.humidity1), label='Sensor 1', color='#4ecdc4', linewidth=2)
            self.ax_humidity.plot(x, list(self.sensor_data.humidity2), label='Sensor 2', color='#7ee8de', linewidth=2, linestyle='--')
            self.ax_humidity.set_title('Humidity (Both Sensors)', fontsize=11, color='white')
            self.ax_humidity.set_ylabel('Humidity (%)', color='white')
            self.ax_humidity.tick_params(colors='white')
            self.ax_humidity.legend(loc='upper left', fontsize=9)
            self.ax_humidity.grid(True, alpha=0.2)
            
            self.fig_temp.canvas.draw()
        
    def update_env_graph(self):
        """Update UV and CO2 graphs"""
        self.ax_uv.clear()
        self.ax_co2.clear()
        
        if len(self.sensor_data.uv) > 0:
            x = range(len(self.sensor_data.uv))
            
            # UV
            self.ax_uv.plot(x, list(self.sensor_data.uv), color='#ffd93d', linewidth=2)
            self.ax_uv.fill_between(x, list(self.sensor_data.uv), alpha=0.3, color='#ffd93d')
            self.ax_uv.set_title('UV Radiation', fontsize=11, color='white')
            self.ax_uv.set_ylabel('W/m²', color='white')
            self.ax_uv.tick_params(colors='white')
            self.ax_uv.grid(True, alpha=0.2)
            
            # CO2
            self.ax_co2.plot(x, list(self.sensor_data.co2), color='#95e1d3', linewidth=2)
            self.ax_co2.fill_between(x, list(self.sensor_data.co2), alpha=0.3, color='#95e1d3')
            self.ax_co2.set_title('CO₂ Level', fontsize=11, color='white')
            self.ax_co2.set_ylabel('ppm', color='white')
            self.ax_co2.tick_params(colors='white')
            self.ax_co2.grid(True, alpha=0.2)
            
            self.fig_env.canvas.draw()
        
    def update_flow_graph(self):
        """Update flow sensor graphs"""
        self.ax_flow.clear()
        
        if len(self.sensor_data.flow1) > 0:
            x = range(len(self.sensor_data.flow1))
            
            self.ax_flow.plot(x, list(self.sensor_data.flow1), label='Pump 1', color='#6c5ce7', linewidth=2)
            self.ax_flow.plot(x, list(self.sensor_data.flow2), label='Pump 2', color='#a29bfe', linewidth=2)
            self.ax_flow.set_title('Microfluidics Flow Rate', fontsize=11, color='white')
            self.ax_flow.set_xlabel('Time', color='white')
            self.ax_flow.set_ylabel('Flow (µL/min)', color='white')
            self.ax_flow.tick_params(colors='white')
            self.ax_flow.legend(loc='upper right', fontsize=9)
            self.ax_flow.grid(True, alpha=0.2)
            
            self.fig_flow.canvas.draw()
        
    def update_sensor_labels(self):
        """Update current sensor value displays"""
        if len(self.sensor_data.temp1) > 0:
            self.labels['temp1'].configure(text=f"{self.sensor_data.temp1[-1]:.1f}")
            self.labels['humidity1'].configure(text=f"{self.sensor_data.humidity1[-1]:.1f}")
            self.labels['temp2'].configure(text=f"{self.sensor_data.temp2[-1]:.1f}")
            self.labels['humidity2'].configure(text=f"{self.sensor_data.humidity2[-1]:.1f}")
            self.labels['uv'].configure(text=f"{self.sensor_data.uv[-1]:.1f}")
            self.labels['co2'].configure(text=f"{self.sensor_data.co2[-1]:.0f}")
            
            self.pump_labels[1].configure(text=f"Current Flow: {self.sensor_data.flow1[-1]:.1f} µL/min")
            self.pump_labels[2].configure(text=f"Current Flow: {self.sensor_data.flow2[-1]:.1f} µL/min")
        
    def update_target_temp(self, value):
        """Update target temperature display"""
        temp = float(value)
        self.temp_value_label.configure(text=f"{temp:.1f}°C")
        self.send_command_to_esp32({'command': 'set_temp', 'value': temp})
        
    def set_temp_from_entry(self):
        """Set temperature from entry widget"""
        try:
            temp = float(self.temp_entry.get())
            if 20 <= temp <= 50:
                self.temp_slider.set(temp)
            else:
                self.show_error("Temperature must be between 20-50°C")
        except ValueError:
            self.show_error("Invalid temperature value")
        
    def toggle_uv_group(self, group_id):
        """Toggle UV group on/off"""
        enabled = self.uv_switches[group_id].get()
        self.send_command_to_esp32({
            'command': 'uv_toggle',
            'group': group_id,
            'enabled': enabled
        })
        
    def update_uv_intensity(self, group_id, value):
        """Update UV group intensity"""
        intensity = int(float(value))
        self.uv_labels[group_id].configure(text=f"{intensity}%")
        self.send_command_to_esp32({
            'command': 'uv_intensity',
            'group': group_id,
            'intensity': intensity
        })
        
    def set_pump_flow(self, pump_id):
        """Set micropump flow rate"""
        try:
            flow = float(self.pump_entries[pump_id].get())
            if 0 <= flow <= 100:
                self.send_command_to_esp32({
                    'command': 'set_flow',
                    'pump': pump_id,
                    'flow': flow
                })
            else:
                self.show_error("Flow must be between 0-100 µL/min")
        except ValueError:
            self.show_error("Invalid flow value")
        
    def connect_to_esp32(self):
        """Connect to ESP32 via WiFi"""
        def connection_thread():
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2)
                result = sock.connect_ex((self.esp32_host, self.esp32_port))
                sock.close()
                
                if result == 0:
                    self.wifi_connected = True
                    self.connection_indicator.configure(text_color="#44ff44")
                    self.connection_label.configure(text="Connected")
                    self.connect_btn.configure(text="Disconnect", command=self.disconnect_from_esp32)
                else:
                    self.show_error("Failed to connect to ESP32")
            except Exception as e:
                self.show_error(f"Connection error: {str(e)}")
        
        thread = threading.Thread(target=connection_thread, daemon=True)
        thread.start()
        
    def disconnect_from_esp32(self):
        """Disconnect from ESP32"""
        self.wifi_connected = False
        self.connection_indicator.configure(text_color="#ff4444")
        self.connection_label.configure(text="Disconnected")
        self.connect_btn.configure(text="Connect ESP32", command=self.connect_to_esp32)
        
    def send_command_to_esp32(self, command):
        """Send command to ESP32"""
        if not self.wifi_connected:
            return
        
        def send_thread():
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2)
                sock.connect((self.esp32_host, self.esp32_port))
                
                # Convert command to JSON and send
                data = json.dumps(command).encode('utf-8')
                sock.sendall(data)
                
                sock.close()
            except Exception as e:
                print(f"Send error: {str(e)}")
        
        thread = threading.Thread(target=send_thread, daemon=True)
        thread.start()
        
    def save_data_to_csv(self):
        """Save sensor data to CSV file in Incubator_Data/YYYYMMDD folder"""
        import csv
        from pathlib import Path
        
        if len(self.sensor_data.temp1) == 0:
            self.show_error("No data to save")
            return
        
        try:
            # Create directory structure
            now = datetime.now()
            date_folder = now.strftime("%Y%m%d")
            data_dir = Path("Incubator_Data") / date_folder
            data_dir.mkdir(parents=True, exist_ok=True)
            
            # Create filename with timestamp
            timestamp = now.strftime("%Y%m%d_%H%M%S")
            filename = f"incubator_data_{timestamp}.csv"
            filepath = data_dir / filename
            
            # Write data to CSV
            with open(filepath, 'w', newline='') as csvfile:
                fieldnames = ['Timestamp', 'Temp1 (°C)', 'Humidity1 (%)', 'Temp2 (°C)', 
                             'Humidity2 (%)', 'UV (W/m²)', 'CO2 (ppm)', 'Flow1 (µL/min)', 'Flow2 (µL/min)']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                
                writer.writeheader()
                for i in range(len(self.sensor_data.temp1)):
                    writer.writerow({
                        'Timestamp': self.sensor_data.timestamps[i].strftime("%Y-%m-%d %H:%M:%S"),
                        'Temp1 (°C)': f"{self.sensor_data.temp1[i]:.2f}",
                        'Humidity1 (%)': f"{self.sensor_data.humidity1[i]:.2f}",
                        'Temp2 (°C)': f"{self.sensor_data.temp2[i]:.2f}",
                        'Humidity2 (%)': f"{self.sensor_data.humidity2[i]:.2f}",
                        'UV (W/m²)': f"{self.sensor_data.uv[i]:.2f}",
                        'CO2 (ppm)': f"{self.sensor_data.co2[i]:.0f}",
                        'Flow1 (µL/min)': f"{self.sensor_data.flow1[i]:.2f}",
                        'Flow2 (µL/min)': f"{self.sensor_data.flow2[i]:.2f}",
                    })
            
            # Show success message
            success_window = ctk.CTkToplevel(self)
            success_window.title("Success")
            success_window.geometry("350x140")
            success_window.resizable(False, False)
            
            msg = ctk.CTkLabel(
                success_window,
                text=f"Data saved successfully!\n\nFolder: Incubator_Data/{date_folder}\nFile: {filename}",
                wraplength=320
            )
            msg.pack(padx=10, pady=15)
            
            btn = ctk.CTkButton(success_window, text="OK", command=success_window.destroy)
            btn.pack(pady=10)
            
        except Exception as e:
            self.show_error(f"Error saving data: {str(e)}")
        
    def start_data_simulation(self):
        """Simulate sensor data for testing"""
        import random
        
        def simulation_thread():
            while True:
                data = {
                    'timestamp': datetime.now(),
                    'temp1': 37 + random.uniform(-0.5, 0.5),
                    'humidity1': 65 + random.uniform(-5, 5),
                    'temp2': 36.8 + random.uniform(-0.5, 0.5),
                    'humidity2': 70 + random.uniform(-5, 5),
                    'uv': 100 + random.uniform(-20, 20),
                    'co2': 400 + random.uniform(-50, 50),
                    'flow1': 25 + random.uniform(-2, 2),
                    'flow2': 30 + random.uniform(-2, 2),
                }
                self.sensor_data.add_data(data)
                threading.Event().wait(1)
        
        thread = threading.Thread(target=simulation_thread, daemon=True)
        thread.start()
        
    def show_error(self, message):
        """Show error message"""
        error_window = ctk.CTkToplevel(self)
        error_window.title("Error")
        error_window.geometry("300x150")
        error_window.resizable(False, False)
        
        label = ctk.CTkLabel(error_window, text=message, wraplength=280)
        label.pack(padx=10, pady=20)
        
        btn = ctk.CTkButton(error_window, text="OK", command=error_window.destroy)
        btn.pack(pady=10)


if __name__ == "__main__":
    app = IncubatorUI()
    app.mainloop()
