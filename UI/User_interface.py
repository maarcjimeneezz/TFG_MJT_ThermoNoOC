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
import time

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

    def clear(self):
        self.timestamps.clear()
        self.temp1.clear()
        self.humidity1.clear()
        self.temp2.clear()
        self.humidity2.clear()
        self.uv.clear()
        self.co2.clear()
        self.flow1.clear()
        self.flow2.clear()


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
        self.esp32_host = "192.168.0.132"
        self.esp32_port = 5000
        self.incubator_closed = False
        self.microfluidics_closed = False
        
        # Current sheet
        self.current_sheet = 0
        self.sheets = []
        
        # Setup UI
        self.setup_ui()
        self.start_data_simulation()
        
    def setup_ui(self):
        """Setup main UI layout"""
        # Configure grid
        self.grid_rowconfigure(0, weight=0)
        self.grid_rowconfigure(1, weight=0)
        self.grid_rowconfigure(2, weight=1)
        self.grid_columnconfigure(0, weight=1)
        
        # Header with clock and connection status
        self.create_header()
        
        # Sheet navigation buttons
        self.create_navigation()
        
        # Main content frame
        self.main_frame = ctk.CTkFrame(self)
        self.main_frame.grid(row=2, column=0, sticky="nsew", padx=10, pady=10)
        self.main_frame.grid_columnconfigure(0, weight=1)
        self.main_frame.grid_rowconfigure(0, weight=1)
        
        # Create sheets
        self.temperature_sheet = TemperatureSheet(self.main_frame, self)
        self.uv_sheet = UVSheet(self.main_frame, self)
        self.microfluidics_sheet = MicrofluidicsSheet(self.main_frame, self)
        
        self.sheets = [self.temperature_sheet, self.uv_sheet, self.microfluidics_sheet]
        
        # Show first sheet
        self.show_sheet(0)
        
        # Start graph updates and clock
        self.setup_graph_updates()
        self.start_clock()
        
    def create_header(self):
        """Create top header with clock and connection status"""
        header_frame = ctk.CTkFrame(self, height=80)
        header_frame.grid(row=0, column=0, sticky="ew", padx=10, pady=10)
        header_frame.grid_columnconfigure(1, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            header_frame,
            text="ThermoNoOC Incubator Control System",
            font=("Helvetica", 24, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=20)
        
        # Clock
        self.clock_label = ctk.CTkLabel(
            header_frame,
            text="00:00:00",
            font=("Helvetica", 18, "bold"),
            text_color="#6c5ce7"
        )
        self.clock_label.grid(row=0, column=1, sticky="e", padx=20)
        
        # Connection status
        connection_frame = ctk.CTkFrame(header_frame)
        connection_frame.grid(row=0, column=2, padx=20)
        
        self.connection_indicator = ctk.CTkLabel(
            connection_frame,
            text="●",
            font=("Helvetica", 16),
            text_color="#ff4444"
        )
        self.connection_indicator.grid(row=0, column=0, padx=5)
        
        self.connection_label = ctk.CTkLabel(
            connection_frame,
            text="Disconnected",
            font=("Helvetica", 12)
        )
        self.connection_label.grid(row=0, column=1, padx=5)
        
        # Connect button
        self.connect_btn = ctk.CTkButton(
            header_frame,
            text="Connect ESP32",
            command=self.connect_to_esp32,
            width=150,
            height=40
        )
        self.connect_btn.grid(row=0, column=3, padx=10)
        
    def create_navigation(self):
        """Create sheet navigation buttons"""
        nav_frame = ctk.CTkFrame(self)
        nav_frame.grid(row=1, column=0, sticky="ew", padx=10, pady=5)
        nav_frame.grid_columnconfigure(0, weight=1)
        
        # Sheet buttons
        buttons_frame = ctk.CTkFrame(nav_frame)
        buttons_frame.grid(row=0, column=0, sticky="w", padx=20)
        
        sheet_names = ["Temperature Control", "UV Control", "Microfluidics Control"]
        self.sheet_buttons = []
        
        for idx, name in enumerate(sheet_names):
            btn = ctk.CTkButton(
                buttons_frame,
                text=name,
                command=lambda i=idx: self.show_sheet(i),
                width=150,
                height=35
            )
            btn.grid(row=0, column=idx, padx=5)
            self.sheet_buttons.append(btn)
        
        # Save data button
        self.save_data_btn = ctk.CTkButton(
            nav_frame,
            text="Save Data to CSV",
            command=self.save_data_to_csv,
            width=150,
            height=35
        )
        self.save_data_btn.grid(row=0, column=1, sticky="e", padx=20)
        
        # Closed status buttons
        closure_frame = ctk.CTkFrame(nav_frame)
        closure_frame.grid(row=1, column=0, columnspan=2, sticky="ew", padx=20, pady=8)
        closure_frame.grid_columnconfigure((0, 1), weight=1)
        
        self.incubator_closed_btn = ctk.CTkButton(
            closure_frame,
            text="Incubator Open",
            command=self.toggle_incubator_closed,
            width=180,
            height=35,
            fg_color="#aa2222"
        )
        self.incubator_closed_btn.grid(row=0, column=0, padx=5)
        
        self.microfluidics_closed_btn = ctk.CTkButton(
            closure_frame,
            text="Microfluidics Open",
            command=self.toggle_microfluidics_closed,
            width=180,
            height=35,
            fg_color="#aa2222"
        )
        self.microfluidics_closed_btn.grid(row=0, column=1, padx=5)
        
    def toggle_incubator_closed(self):
        self.incubator_closed = not self.incubator_closed
        if self.incubator_closed:
            self.incubator_closed_btn.configure(text="Incubator Closed", fg_color="#229922")
        else:
            self.incubator_closed_btn.configure(text="Incubator Open", fg_color="#aa2222")

    def toggle_microfluidics_closed(self):
        self.microfluidics_closed = not self.microfluidics_closed
        if self.microfluidics_closed:
            self.microfluidics_closed_btn.configure(text="Microfluidics Closed", fg_color="#229922")
        else:
            self.microfluidics_closed_btn.configure(text="Microfluidics Open", fg_color="#aa2222")

    def show_sheet(self, sheet_index):
        """Show the specified sheet"""
        # Hide all sheets
        for sheet in self.sheets:
            sheet.frame.grid_forget()
        
        # Show selected sheet
        self.sheets[sheet_index].frame.grid(row=0, column=0, sticky="nsew")
        self.current_sheet = sheet_index
        
        # Update button colors
        for idx, btn in enumerate(self.sheet_buttons):
            if idx == sheet_index:
                btn.configure(fg_color="#0066ff")
            else:
                btn.configure(fg_color="#1f1f1f")
        
    def setup_graph_updates(self):
        """Setup graph animation"""
        def update_graphs():
            while True:
                self.temperature_sheet.update_graphs()
                self.uv_sheet.update_graphs()
                self.microfluidics_sheet.update_graphs()
                self.after(1000)
        
        thread = threading.Thread(target=update_graphs, daemon=True)
        thread.start()
        
    def start_clock(self):
        """Update clock every second"""
        def update_clock():
            while True:
                current_time = datetime.now().strftime("%H:%M:%S")
                self.clock_label.configure(text=current_time)
                time.sleep(1)
        
        thread = threading.Thread(target=update_clock, daemon=True)
        thread.start()
        
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
        """Save sensor data to CSV file"""
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
                fieldnames = ['Timestamp', 'Temp1 (C)', 'Humidity1 (%)', 'Temp2 (C)', 
                             'Humidity2 (%)', 'UV (W/m2)', 'CO2 (%)', 'Flow1 (uL/min)', 'Flow2 (uL/min)']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                
                writer.writeheader()
                for i in range(len(self.sensor_data.temp1)):
                    writer.writerow({
                        'Timestamp': self.sensor_data.timestamps[i].strftime("%Y-%m-%d %H:%M:%S"),
                        'Temp1 (C)': f"{self.sensor_data.temp1[i]:.2f}",
                        'Humidity1 (%)': f"{self.sensor_data.humidity1[i]:.2f}",
                        'Temp2 (C)': f"{self.sensor_data.temp2[i]:.2f}",
                        'Humidity2 (%)': f"{self.sensor_data.humidity2[i]:.2f}",
                        'UV (W/m2)': f"{self.sensor_data.uv[i]:.2f}",
                        'CO2 (%)': f"{(self.sensor_data.co2[i] / 10000):.4f}",
                        'Flow1 (uL/min)': f"{self.sensor_data.flow1[i]:.2f}",
                        'Flow2 (uL/min)': f"{self.sensor_data.flow2[i]:.2f}",
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
                data = {'timestamp': datetime.now()}
                
                if self.incubator_closed:
                    data.update({
                        'temp1': 37 + random.uniform(-0.5, 0.5),
                        'humidity1': 65 + random.uniform(-5, 5),
                        'temp2': 36.8 + random.uniform(-0.5, 0.5),
                        'humidity2': 70 + random.uniform(-5, 5),
                        'uv': 100 + random.uniform(-20, 20),
                        'co2': 400 + random.uniform(-50, 50),
                    })
                else:
                    # Clear incubator data
                    if len(self.sensor_data.temp1) > 0:
                        self.sensor_data.temp1.clear()
                        self.sensor_data.humidity1.clear()
                        self.sensor_data.temp2.clear()
                        self.sensor_data.humidity2.clear()
                        self.sensor_data.uv.clear()
                        self.sensor_data.co2.clear()
                
                if self.microfluidics_closed:
                    data.update({
                        'flow1': 25 + random.uniform(-2, 2),
                        'flow2': 30 + random.uniform(-2, 2),
                    })
                else:
                    # Clear microfluidics data
                    if len(self.sensor_data.flow1) > 0:
                        self.sensor_data.flow1.clear()
                        self.sensor_data.flow2.clear()
                
                if data:
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


class TemperatureSheet:
    """Sheet for temperature control and monitoring"""
    
    def __init__(self, parent, main_app):
        self.main_app = main_app
        
        # Main frame
        self.frame = ctk.CTkFrame(parent)
        self.frame.grid_columnconfigure(0, weight=1)
        self.frame.grid_rowconfigure((0, 1, 2), weight=0)
        self.frame.grid_rowconfigure(3, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            self.frame,
            text="Temperature Control",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=15, pady=10)
        
        # Temperature and Humidity Graph
        self.fig_temp = Figure(figsize=(14, 3.5), dpi=100)
        self.ax_temp = self.fig_temp.add_subplot(121)
        self.ax_humidity = self.fig_temp.add_subplot(122)
        self.fig_temp.patch.set_facecolor('#212121')
        
        canvas_temp = FigureCanvasTkAgg(self.fig_temp, self.frame)
        canvas_temp.get_tk_widget().grid(row=1, column=0, sticky="nsew", padx=10, pady=10)
        
        # CO2 Graph
        self.fig_co2 = Figure(figsize=(14, 3), dpi=100)
        self.ax_co2 = self.fig_co2.add_subplot(111)
        self.fig_co2.patch.set_facecolor('#212121')
        
        canvas_co2 = FigureCanvasTkAgg(self.fig_co2, self.frame)
        canvas_co2.get_tk_widget().grid(row=2, column=0, sticky="nsew", padx=10, pady=10)
        
        # Control section
        control_frame = ctk.CTkFrame(self.frame)
        control_frame.grid(row=3, column=0, sticky="ew", padx=15, pady=15)
        control_frame.grid_columnconfigure(1, weight=1)
        
        # Temperature control
        temp_label = ctk.CTkLabel(control_frame, text="Target Temperature:", font=("Helvetica", 12, "bold"))
        temp_label.grid(row=0, column=0, sticky="w", padx=10, pady=10)
        
        self.temp_slider = ctk.CTkSlider(
            control_frame,
            from_=20,
            to=50,
            number_of_steps=150,
            command=self.update_target_temp,
            height=8
        )
        self.temp_slider.set(20)
        self.temp_slider.grid(row=0, column=1, sticky="ew", padx=20, pady=10)
        
        self.temp_value_label = ctk.CTkLabel(
            control_frame,
            text="20.0 C",
            font=("Helvetica", 14, "bold"),
            text_color="#00ff00",
            width=80
        )
        self.temp_value_label.grid(row=0, column=2, padx=10, pady=10)
        
        # Entry field
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
        
        # Current values display
        values_frame = ctk.CTkFrame(self.frame)
        values_frame.grid(row=4, column=0, sticky="ew", padx=10, pady=10)
        values_frame.grid_columnconfigure((0, 1, 2, 3, 4), weight=1)
        
        sensors = [
            ('temp1', 'Glass sensor temp', 'C'),
            ('humidity1', 'Glass sensor humidity', '%'),
            ('temp2', 'Base sensor temp', 'C'),
            ('humidity2', 'Base sensor humidity', '%'),
            ('co2', 'CO2', '%')
        ]
        
        self.labels = {}
        for idx, (key, label, unit) in enumerate(sensors):
            frame = ctk.CTkFrame(values_frame)
            frame.grid(row=0, column=idx, sticky="nsew", padx=5)
            
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
        
    def update_graphs(self):
        """Update temperature graphs"""
        self.ax_temp.clear()
        self.ax_humidity.clear()
        self.ax_co2.clear()
        
        if len(self.main_app.sensor_data.temp1) > 0:
            x = range(len(self.main_app.sensor_data.temp1))
            
            # Temperature
            self.ax_temp.plot(x, list(self.main_app.sensor_data.temp1), label='Glass sensor', color='#ff6b6b', linewidth=2)
            self.ax_temp.plot(x, list(self.main_app.sensor_data.temp2), label='Base sensor', color='#ff9999', linewidth=2, linestyle='--')
            self.ax_temp.set_title('Temperature', fontsize=11, color='white')
            self.ax_temp.set_ylabel('Temperature (C)', color='white')
            self.ax_temp.tick_params(colors='white')
            self.ax_temp.legend(loc='upper left', fontsize=9)
            self.ax_temp.grid(True, alpha=0.2)
            
            # Set y-axis limits for temperature
            if len(self.main_app.sensor_data.temp1) > 0:
                self.ax_temp.set_ylim(20, 50)
            
            # Humidity
            self.ax_humidity.plot(x, list(self.main_app.sensor_data.humidity1), label='Glass sensor', color='#4ecdc4', linewidth=2)
            self.ax_humidity.plot(x, list(self.main_app.sensor_data.humidity2), label='Base sensor', color='#7ee8de', linewidth=2, linestyle='--')
            self.ax_humidity.set_title('Humidity', fontsize=11, color='white')
            self.ax_humidity.set_ylabel('Humidity (%)', color='white')
            self.ax_humidity.tick_params(colors='white')
            self.ax_humidity.legend(loc='upper left', fontsize=9)
            self.ax_humidity.grid(True, alpha=0.2)
            
            # Set y-axis limits for humidity
            if len(self.main_app.sensor_data.humidity1) > 0:
                self.ax_humidity.set_ylim(0, 100)
            
            # CO2
            co2_percent = [value / 10000.0 for value in self.main_app.sensor_data.co2]
            self.ax_co2.plot(x, co2_percent, color='#95e1d3', linewidth=2)
            self.ax_co2.fill_between(x, co2_percent, alpha=0.3, color='#95e1d3')
            self.ax_co2.set_title('CO2 Level', fontsize=11, color='white')
            self.ax_co2.set_ylabel('CO2 (%)', color='white')
            self.ax_co2.tick_params(colors='white')
            self.ax_co2.grid(True, alpha=0.2)
            
            # Set y-axis limits for CO2
            if len(self.main_app.sensor_data.co2) > 0:
                self.ax_co2.set_ylim(0, 10)
            
            self.fig_temp.canvas.draw()
            self.fig_co2.canvas.draw()
            
            # Update sensor labels
            if len(self.main_app.sensor_data.temp1) > 0:
                self.labels['temp1'].configure(text=f"{self.main_app.sensor_data.temp1[-1]:.1f}")
                self.labels['humidity1'].configure(text=f"{self.main_app.sensor_data.humidity1[-1]:.1f}")
                self.labels['temp2'].configure(text=f"{self.main_app.sensor_data.temp2[-1]:.1f}")
                self.labels['humidity2'].configure(text=f"{self.main_app.sensor_data.humidity2[-1]:.1f}")
                self.labels['co2'].configure(text=f"{(self.main_app.sensor_data.co2[-1] / 10000.0):.4f}")
        else:
            self.ax_temp.axis('off')
            self.ax_humidity.axis('off')
            self.ax_co2.axis('off')
            self.ax_temp.text(0.5, 0.5, 'No readings\nClose both incubator and microfluidics', ha='center', va='center', color='white', fontsize=14)
            self.fig_temp.canvas.draw()
            self.fig_co2.canvas.draw()
            for key in self.labels:
                self.labels[key].configure(text="---")
        
    def update_target_temp(self, value):
        """Update target temperature display"""
        temp = float(value)
        self.temp_value_label.configure(text=f"{temp:.1f} C")
        self.main_app.send_command_to_esp32({'command': 'set_temp', 'value': temp})
        
    def set_temp_from_entry(self):
        """Set temperature from entry widget"""
        try:
            temp = float(self.temp_entry.get())
            if 20 <= temp <= 50:
                self.temp_slider.set(temp)
            else:
                self.main_app.show_error("Temperature must be between 20-50 C")
        except ValueError:
            self.main_app.show_error("Invalid temperature value")


class UVSheet:
    """Sheet for UV control and monitoring"""
    
    def __init__(self, parent, main_app):
        self.main_app = main_app
        
        # Main frame
        self.frame = ctk.CTkFrame(parent)
        self.frame.grid_columnconfigure(0, weight=1)
        self.frame.grid_rowconfigure((0, 1, 2), weight=0)
        self.frame.grid_rowconfigure(3, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            self.frame,
            text="UV Control",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=15, pady=10)
        
        # UV Graph
        self.fig_uv = Figure(figsize=(14, 4), dpi=100)
        self.ax_uv = self.fig_uv.add_subplot(111)
        self.fig_uv.patch.set_facecolor('#212121')
        
        canvas_uv = FigureCanvasTkAgg(self.fig_uv, self.frame)
        canvas_uv.get_tk_widget().grid(row=1, column=0, sticky="nsew", padx=10, pady=10)
        
        # UV controls
        control_frame = ctk.CTkFrame(self.frame)
        control_frame.grid(row=2, column=0, sticky="ew", padx=15, pady=15)
        control_frame.grid_columnconfigure((0, 1, 2, 3), weight=1)
        
        self.uv_sliders = {}
        self.uv_labels = {}
        self.uv_switches = {}
        
        for i in range(1, 5):
            # Frame for each LED group
            frame = ctk.CTkFrame(control_frame)
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
        
        # Current UV value display
        uv_value_frame = ctk.CTkFrame(self.frame)
        uv_value_frame.grid(row=3, column=0, sticky="ew", padx=10, pady=10)
        uv_value_frame.grid_columnconfigure(0, weight=1)
        
        self.uv_current_label = ctk.CTkLabel(
            uv_value_frame,
            text="Current UV: 0.0 W/m2",
            font=("Helvetica", 16, "bold"),
            text_color="#ffd93d"
        )
        self.uv_current_label.grid(row=0, column=0, pady=10)
        
    def update_graphs(self):
        """Update UV graphs"""
        self.ax_uv.clear()
        
        if len(self.main_app.sensor_data.uv) > 0:
            x = range(len(self.main_app.sensor_data.uv))
            
            self.ax_uv.plot(x, list(self.main_app.sensor_data.uv), color='#ffd93d', linewidth=2)
            self.ax_uv.fill_between(x, list(self.main_app.sensor_data.uv), alpha=0.3, color='#ffd93d')
            self.ax_uv.set_title('UV Radiation', fontsize=12, color='white')
            self.ax_uv.set_xlabel('Time', color='white')
            self.ax_uv.set_ylabel('W/m2', color='white')
            self.ax_uv.tick_params(colors='white')
            self.ax_uv.grid(True, alpha=0.2)
            
            self.fig_uv.canvas.draw()
            
            # Update UV label
            self.uv_current_label.configure(text=f"Current UV: {self.main_app.sensor_data.uv[-1]:.1f} W/m2")
            
    def toggle_uv_group(self, group_id):
        """Toggle UV group on/off"""
        enabled = self.uv_switches[group_id].get()
        self.main_app.send_command_to_esp32({
            'command': 'uv_toggle',
            'group': group_id,
            'enabled': enabled
        })
        
    def update_uv_intensity(self, group_id, value):
        """Update UV group intensity"""
        intensity = int(float(value))
        self.uv_labels[group_id].configure(text=f"{intensity}%")
        self.main_app.send_command_to_esp32({
            'command': 'uv_intensity',
            'group': group_id,
            'intensity': intensity
        })


class MicrofluidicsSheet:
    """Sheet for microfluidics control and monitoring"""
    
    def __init__(self, parent, main_app):
        self.main_app = main_app
        
        # Main frame
        self.frame = ctk.CTkFrame(parent)
        self.frame.grid_columnconfigure(0, weight=1)
        self.frame.grid_rowconfigure((0, 1, 2), weight=0)
        self.frame.grid_rowconfigure(3, weight=1)
        
        # Title
        title_label = ctk.CTkLabel(
            self.frame,
            text="Microfluidics Control",
            font=("Helvetica", 16, "bold")
        )
        title_label.grid(row=0, column=0, sticky="w", padx=15, pady=10)
        
        # Flow sensors graph
        self.fig_flow = Figure(figsize=(14, 4), dpi=100)
        self.ax_flow = self.fig_flow.add_subplot(111)
        self.fig_flow.patch.set_facecolor('#212121')
        
        canvas_flow = FigureCanvasTkAgg(self.fig_flow, self.frame)
        canvas_flow.get_tk_widget().grid(row=1, column=0, sticky="nsew", padx=10, pady=10)
        
        # Pump controls frame
        pumps_frame = ctk.CTkFrame(self.frame)
        pumps_frame.grid(row=2, column=0, sticky="ew", padx=15, pady=15)
        pumps_frame.grid_columnconfigure((0, 1), weight=1)
        
        self.pump_entries = {}
        self.pump_labels = {}
        self.pump_modes = {}
        self.pump_feeding_entries = {}
        self.pump_pause_entries = {}
        self.pump_cycles_entries = {}
        
        for pump_id in range(1, 3):
            # Frame for each pump
            frame = ctk.CTkFrame(pumps_frame)
            frame.grid(row=0, column=pump_id-1, sticky="nsew", padx=10, pady=10)
            frame.grid_columnconfigure(1, weight=1)
            frame.grid_rowconfigure((0, 1, 2, 3, 4, 5, 6, 7, 8), weight=0)
            
            # Title
            pump_label = ctk.CTkLabel(
                frame,
                text=f"Micropump {pump_id}",
                font=("Helvetica", 12, "bold")
            )
            pump_label.grid(row=0, column=0, columnspan=2, sticky="w", pady=5)
            
            # Flow target
            flow_label = ctk.CTkLabel(frame, text="Target Flow (uL):", font=("Helvetica", 10))
            flow_label.grid(row=1, column=0, sticky="w", pady=5)
            
            entry = ctk.CTkEntry(
                frame,
                placeholder_text="0-2000",
                width=120
            )
            entry.insert(0, "0")
            entry.grid(row=1, column=1, sticky="w", padx=10, pady=5)
            self.pump_entries[pump_id] = entry
            
            # Current flow display
            current_label = ctk.CTkLabel(
                frame,
                text="Current Flow: 0.0 uL/min",
                font=("Helvetica", 10),
                text_color="#00ff00"
            )
            current_label.grid(row=2, column=0, columnspan=2, sticky="w", pady=5)
            self.pump_labels[pump_id] = current_label
            
            # Mode buttons
            mode_frame = ctk.CTkFrame(frame)
            mode_frame.grid(row=3, column=0, columnspan=2, sticky="ew", pady=5)
            mode_frame.grid_columnconfigure((0, 1), weight=1)
            
            continuous_btn = ctk.CTkButton(
                mode_frame,
                text="Continuous",
                command=lambda idx=pump_id: self.set_pump_mode(idx, 'continuous'),
                width=80,
                fg_color="#229922"
            )
            continuous_btn.grid(row=0, column=0, padx=2)
            
            intermittent_btn = ctk.CTkButton(
                mode_frame,
                text="Intermittent",
                command=lambda idx=pump_id: self.set_pump_mode(idx, 'intermittent'),
                width=80,
                fg_color="#1f1f1f"
            )
            intermittent_btn.grid(row=0, column=1, padx=2)
            
            self.pump_modes[pump_id] = {'mode': 'continuous', 'continuous_btn': continuous_btn, 'intermittent_btn': intermittent_btn}
            
            # Timing entries
            feeding_entry = ctk.CTkEntry(frame, placeholder_text="Feeding time (s)", width=100)
            feeding_entry.grid(row=4, column=0, sticky="w", padx=5, pady=2)
            feeding_entry.configure(state="disabled")  # Disabled by default (continuous mode)
            self.pump_feeding_entries[pump_id] = feeding_entry
            
            pause_entry = ctk.CTkEntry(frame, placeholder_text="Pause time (s)", width=100)
            pause_entry.grid(row=4, column=1, sticky="w", padx=5, pady=2)
            pause_entry.configure(state="disabled")  # Disabled by default (continuous mode)
            self.pump_pause_entries[pump_id] = pause_entry
            
            cycles_entry = ctk.CTkEntry(frame, placeholder_text="Cycles", width=100)
            cycles_entry.grid(row=5, column=0, columnspan=2, sticky="ew", padx=5, pady=2)
            cycles_entry.configure(state="disabled")  # Disabled by default (continuous mode)
            self.pump_cycles_entries[pump_id] = cycles_entry
            
            # Set button
            set_btn = ctk.CTkButton(
                frame,
                text="Set Flow & Mode",
                command=lambda idx=pump_id: self.set_pump_flow_and_mode(idx),
                width=120
            )
            set_btn.grid(row=6, column=0, columnspan=2, sticky="ew", pady=10)
        
    def update_graphs(self):
        """Update flow graph"""
        self.ax_flow.clear()
        
        if len(self.main_app.sensor_data.flow1) > 0:
            x = range(len(self.main_app.sensor_data.flow1))
            
            self.ax_flow.plot(x, list(self.main_app.sensor_data.flow1), label='Pump 1', color='#6c5ce7', linewidth=2)
            self.ax_flow.plot(x, list(self.main_app.sensor_data.flow2), label='Pump 2', color='#a29bfe', linewidth=2)
            self.ax_flow.set_title('Microfluidics Flow Rate', fontsize=12, color='white')
            self.ax_flow.set_xlabel('Time', color='white')
            self.ax_flow.set_ylabel('Flow (uL/min)', color='white')
            self.ax_flow.tick_params(colors='white')
            self.ax_flow.legend(loc='upper right', fontsize=9)
            self.ax_flow.grid(True, alpha=0.2)
            
            self.fig_flow.canvas.draw()
            
            # Update flow labels
            self.pump_labels[1].configure(text=f"Current Flow: {self.main_app.sensor_data.flow1[-1]:.1f} uL/min")
            self.pump_labels[2].configure(text=f"Current Flow: {self.main_app.sensor_data.flow2[-1]:.1f} uL/min")
        else:
            self.ax_flow.axis('off')
            self.ax_flow.text(0.5, 0.5, 'No flow readings\nClose both incubator and microfluidics', ha='center', va='center', color='white', fontsize=14)
            self.fig_flow.canvas.draw()
            self.pump_labels[1].configure(text="Current Flow: ---")
            self.pump_labels[2].configure(text="Current Flow: ---")
            
    def set_pump_flow_and_mode(self, pump_id):
        """Set micropump flow rate and mode"""
        try:
            flow = float(self.pump_entries[pump_id].get())
            if 0 <= flow <= 2000:
                mode = self.pump_modes[pump_id]['mode']
                feeding = float(self.pump_feeding_entries[pump_id].get() or 0)
                pause = float(self.pump_pause_entries[pump_id].get() or 0)
                cycles = int(float(self.pump_cycles_entries[pump_id].get() or 0))
                self.main_app.send_command_to_esp32({
                    'command': 'set_pump_flow_and_mode',
                    'pump': pump_id,
                    'flow': flow,
                    'mode': mode,
                    'feeding_time': feeding,
                    'pause_time': pause,
                    'cycles': cycles
                })
            else:
                self.main_app.show_error("Flow must be between 0-2000 uL")
        except ValueError:
            self.main_app.show_error("Invalid flow or timing values")
        
    def set_pump_mode(self, pump_id, mode):
        self.pump_modes[pump_id]['mode'] = mode
        self.pump_modes[pump_id]['continuous_btn'].configure(fg_color="#229922" if mode == 'continuous' else '#1f1f1f')
        self.pump_modes[pump_id]['intermittent_btn'].configure(fg_color="#229922" if mode == 'intermittent' else '#1f1f1f')
        
        # Enable/disable timing entries based on mode
        state = "normal" if mode == 'intermittent' else "disabled"
        self.pump_feeding_entries[pump_id].configure(state=state)
        self.pump_pause_entries[pump_id].configure(state=state)
        self.pump_cycles_entries[pump_id].configure(state=state)


if __name__ == "__main__":
    app = IncubatorUI()
    app.mainloop()
