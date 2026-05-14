"""
ThermoNoOC — Desktop Control Panel v2

ESP32 Access Point
    SSID:     ThermoNoOC
    Password: thermonooc
    IP:       192.168.4.1
    WS URL:   ws://192.168.4.1:5000

Telemetry JSON (500 ms cadence):
    temp1, temp2   – temperature °C (SHT35 x2)
    hum1,  hum2    – humidity %    (SHT35 x2)
    uvIndex        – UV index      (LTR390)
    uvW            – irradiance W/m²
    co2            – CO₂ %        (T6615)
    flow1, flow2   – µL/min       (microfluidics)

Commands sent:
    SET_TEMP:<float>                          – temperature setpoint
    SET_LED:<group>:<0|1>:<0-100>             – UV LED group
    SET_PUMP:<cir>:<flow>:<pulsed>:<feed>:<pause>:<cycles>

Dependencies:
    pip install customtkinter matplotlib websocket-client
"""

from __future__ import annotations

import json
import threading
from collections import deque
from typing import Callable, Optional

import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

import customtkinter as ctk
import websocket  # websocket-client package

# ─────────────────────────────────────────────────────────────────────────────
# Constants
# ─────────────────────────────────────────────────────────────────────────────
WS_URL     = "ws://192.168.4.1:5000"
BUFFER_LEN = 120      # 60 s of history at 500 ms/sample
REDRAW_MS  = 500      # chart refresh cadence (ms)

# Per-series colours – visible on both dark and light backgrounds
CLR = {
    "temp1": "#FF6B6B",
    "temp2": "#FF9F43",
    "hum1":  "#26de81",
    "hum2":  "#4FC3F7",
    "co2":   "#A29BFE",
    "uv":    "#FD79A8",
}

# Chart theme palettes
_DARK  = dict(bg="#1e1e2e", ax_bg="#252535", fg="#d4d4e8", grid="#353560")
_LIGHT = dict(bg="#f0f4f8", ax_bg="#ffffff",  fg="#1a1a2e", grid="#d0d8e8")


# ─────────────────────────────────────────────────────────────────────────────
# WebSocket client — runs in a daemon thread; all callbacks fire from that thread
# ─────────────────────────────────────────────────────────────────────────────
class WSClient:
    def __init__(self) -> None:
        self._ws:     Optional[websocket.WebSocketApp] = None
        self._thread: Optional[threading.Thread]       = None
        self.connected: bool = False
        self.on_data:   Optional[Callable[[dict], None]] = None
        self.on_status: Optional[Callable[[bool], None]] = None

    def connect(self, url: str) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._thread = threading.Thread(target=self._run, args=(url,), daemon=True)
        self._thread.start()

    def disconnect(self) -> None:
        if self._ws:
            self._ws.close()

    def send(self, msg: str) -> None:
        if self._ws and self.connected:
            try:
                self._ws.send(msg)
            except Exception:
                pass

    # private ──────────────────────────────────────────────────────────────────
    def _run(self, url: str) -> None:
        self._ws = websocket.WebSocketApp(
            url,
            on_open    = self._cb_open,
            on_message = self._cb_message,
            on_error   = self._cb_error,
            on_close   = self._cb_close,
        )
        self._ws.run_forever(ping_interval=10, ping_timeout=5)

    def _cb_open(self, ws):
        self.connected = True
        if self.on_status:
            self.on_status(True)

    def _cb_message(self, ws, raw: str):
        try:
            if self.on_data:
                self.on_data(json.loads(raw))
        except (json.JSONDecodeError, TypeError):
            pass

    def _cb_error(self, ws, err):
        pass

    def _cb_close(self, ws, code, msg):
        self.connected = False
        if self.on_status:
            self.on_status(False)


# ─────────────────────────────────────────────────────────────────────────────
# Send-data dialog
# ─────────────────────────────────────────────────────────────────────────────
class SendDialog(ctk.CTkToplevel):
    def __init__(self, parent: ctk.CTk, ws: WSClient) -> None:
        super().__init__(parent)
        self._ws = ws
        self.title("Send to ESP32")
        self.resizable(False, False)
        self.grab_set()
        self.lift()
        self.focus_force()

        P = {"padx": 20, "pady": 6}

        # Temperature setpoint
        ctk.CTkLabel(self, text="Temperature Setpoint",
                     font=ctk.CTkFont(size=12, weight="bold")).pack(anchor="w", **P)
        row_t = ctk.CTkFrame(self, fg_color="transparent")
        row_t.pack(anchor="w", padx=20, pady=(0, 14))
        self._temp_var = ctk.StringVar(value="37.0")
        ctk.CTkEntry(row_t, textvariable=self._temp_var, width=90).pack(side="left", padx=(0, 6))
        ctk.CTkLabel(row_t, text="°C").pack(side="left")

        # UV LED groups
        ctk.CTkLabel(self, text="UV LED Groups",
                     font=ctk.CTkFont(size=12, weight="bold")).pack(anchor="w", **P)

        self._led_en:  list[ctk.BooleanVar] = []
        self._led_int: list[ctk.IntVar]     = []

        for i in range(4):
            row = ctk.CTkFrame(self, fg_color="transparent")
            row.pack(fill="x", padx=20, pady=3)
            ctk.CTkLabel(row, text=f"Group {i + 1}", width=68, anchor="w").pack(side="left")

            var_en = ctk.BooleanVar(value=False)
            ctk.CTkSwitch(row, variable=var_en, text="",
                          width=44, height=22).pack(side="left", padx=(4, 10))
            self._led_en.append(var_en)

            var_int = ctk.IntVar(value=50)
            ctk.CTkSlider(row, from_=0, to=100,
                          variable=var_int, width=150).pack(side="left")
            self._led_int.append(var_int)

            pct = ctk.CTkLabel(row, text=f"{var_int.get()}%", width=38)
            pct.pack(side="left", padx=(6, 0))
            var_int.trace_add(
                "write",
                lambda *_, v=var_int, lbl=pct: lbl.configure(text=f"{v.get()}%")
            )

        # Buttons
        btn_row = ctk.CTkFrame(self, fg_color="transparent")
        btn_row.pack(padx=20, pady=(14, 18), anchor="e")
        ctk.CTkButton(btn_row, text="Send",   width=88, command=self._send).pack(side="left", padx=4)
        ctk.CTkButton(btn_row, text="Cancel", width=88, command=self.destroy,
                      fg_color="transparent", border_width=1).pack(side="left", padx=4)

    def _send(self) -> None:
        if not self._ws.connected:
            return
        try:
            self._ws.send(f"SET_TEMP:{float(self._temp_var.get()):.1f}")
        except ValueError:
            pass
        for i, (en, inten) in enumerate(zip(self._led_en, self._led_int)):
            self._ws.send(f"SET_LED:{i + 1}:{int(en.get())}:{inten.get()}")
        self.destroy()


# ─────────────────────────────────────────────────────────────────────────────
# Chart utilities
# ─────────────────────────────────────────────────────────────────────────────
def _pal() -> dict:
    return _DARK if ctk.get_appearance_mode().lower() == "dark" else _LIGHT


def _style_ax(ax, pal: dict, ylabel: str = "") -> None:
    ax.set_facecolor(pal["ax_bg"])
    ax.tick_params(colors=pal["fg"], labelsize=8)
    ax.set_ylabel(ylabel, color=pal["fg"], fontsize=9)
    ax.set_xlabel("Time (s)", color=pal["fg"], fontsize=8)
    for sp in ax.spines.values():
        sp.set_edgecolor(pal["grid"])
    ax.grid(True, color=pal["grid"], linewidth=0.4, linestyle="--", alpha=0.7)


def _make_fig(height: float = 2.8) -> tuple:
    pal = _pal()
    fig, ax = plt.subplots(figsize=(9, height), dpi=96)
    fig.patch.set_facecolor(pal["bg"])
    fig.subplots_adjust(left=0.08, right=0.97, top=0.94, bottom=0.16)
    _style_ax(ax, pal)
    return fig, ax


def _draw_series(ax, t: list, series: list, pal: dict, ylabel: str = "") -> None:
    ax.cla()
    _style_ax(ax, pal, ylabel)
    for y, label, color in series:
        ax.plot(t, y, color=color, linewidth=1.6, label=label, solid_capstyle="round")
    if len(series) > 1:
        ax.legend(fontsize=8, framealpha=0.25,
                  facecolor=pal["ax_bg"], labelcolor=pal["fg"],
                  edgecolor=pal["grid"])


# ─────────────────────────────────────────────────────────────────────────────
# Main application
# ─────────────────────────────────────────────────────────────────────────────
class App(ctk.CTk):
    # ── init ──────────────────────────────────────────────────────────────────
    def __init__(self) -> None:
        super().__init__()
        self.title("ThermoNoOC — Control Panel")
        self.geometry("1140x760")
        self.minsize(860, 580)

        ctk.set_appearance_mode("Dark")
        ctk.set_default_color_theme("blue")

        # data buffers
        self._lock    = threading.Lock()
        self._elapsed = 0.0
        self._t: deque = deque(
            [round(-0.5 * (BUFFER_LEN - 1 - i), 1) for i in range(BUFFER_LEN)],
            maxlen=BUFFER_LEN,
        )
        self._bufs: dict[str, deque] = {
            k: deque([0.0] * BUFFER_LEN, maxlen=BUFFER_LEN)
            for k in ("temp1", "temp2", "hum1", "hum2", "co2", "uv_irr", "uv_idx")
        }

        # websocket
        self._ws           = WSClient()
        self._ws.on_data   = self._ingest
        self._ws.on_status = lambda ok: self.after(0, self._set_connected, ok)

        # build
        self._active = 0
        self._build()
        self.after(REDRAW_MS, self._tick)

    # ── layout skeleton ───────────────────────────────────────────────────────
    def _build(self) -> None:
        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)
        self._build_header()
        self._build_content()

    # ── header bar ────────────────────────────────────────────────────────────
    def _build_header(self) -> None:
        hdr = ctk.CTkFrame(self, height=56, corner_radius=0,
                           fg_color=("gray88", "gray13"))
        hdr.grid(row=0, column=0, sticky="ew")
        hdr.grid_propagate(False)
        # elastic spacer at column 4 pushes controls to the right
        hdr.grid_columnconfigure(4, weight=1)

        TAB_NAMES = ["  Incubator  ", "  UV Light  ", "  Microfluidics  "]
        self._tab_btns: list[ctk.CTkButton] = []
        for i, name in enumerate(TAB_NAMES):
            b = ctk.CTkButton(
                hdr, text=name, width=148, height=36, corner_radius=6,
                fg_color=("gray78", "gray23"),
                hover_color=("gray68", "gray32"),
                command=lambda i=i: self._show(i),
            )
            b.grid(row=0, column=i, padx=(10 if i == 0 else 3, 0), pady=10)
            self._tab_btns.append(b)

        # status indicator
        self._dot = ctk.CTkLabel(hdr, text="●",
                                 font=ctk.CTkFont(size=17), text_color="gray42")
        self._dot.grid(row=0, column=5, padx=(14, 2))

        self._conn_lbl = ctk.CTkLabel(hdr, text="Disconnected",
                                      font=ctk.CTkFont(size=11))
        self._conn_lbl.grid(row=0, column=6, padx=(0, 16))

        # connect / disconnect
        self._conn_btn = ctk.CTkButton(
            hdr, text="Connect", width=100, height=34, corner_radius=6,
            command=self._toggle_connect,
        )
        self._conn_btn.grid(row=0, column=7, padx=4)

        # send data
        ctk.CTkButton(
            hdr, text="Send Data", width=100, height=34, corner_radius=6,
            fg_color=("gray78", "gray23"), hover_color=("gray68", "gray32"),
            command=self._open_send,
        ).grid(row=0, column=8, padx=4)

        # theme toggle
        self._theme_btn = ctk.CTkButton(
            hdr, text="☀", width=40, height=34, corner_radius=6,
            fg_color=("gray78", "gray23"), hover_color=("gray68", "gray32"),
            font=ctk.CTkFont(size=15),
            command=self._toggle_theme,
        )
        self._theme_btn.grid(row=0, column=9, padx=(4, 12))

        self._highlight_tab()

    # ── scrollable content area ───────────────────────────────────────────────
    def _build_content(self) -> None:
        self._scroll = ctk.CTkScrollableFrame(
            self, corner_radius=0, fg_color="transparent"
        )
        self._scroll.grid(row=1, column=0, sticky="nsew")
        self._scroll.grid_columnconfigure(0, weight=1)

        self._sheets = [
            self._build_incubator_sheet(),
            self._build_uv_sheet(),
            self._build_micro_sheet(),
        ]
        self._show(0)

    # ── Sheet 1: Incubator ────────────────────────────────────────────────────
    def _build_incubator_sheet(self) -> ctk.CTkFrame:
        fr = ctk.CTkFrame(self._scroll, fg_color="transparent")
        fr.grid_columnconfigure(0, weight=1)

        ctk.CTkLabel(fr, text="Incubator Monitoring",
                     font=ctk.CTkFont(size=18, weight="bold")).grid(
            row=0, column=0, padx=20, pady=(18, 6), sticky="w")

        # Temperature card
        tc = self._card(fr, row=1, title="Temperature (°C)")
        self._temp_fig, self._temp_ax = _make_fig(2.8)
        self._temp_canvas = FigureCanvasTkAgg(self._temp_fig, master=tc)
        self._temp_canvas.draw()
        self._temp_canvas.get_tk_widget().pack(fill="x", padx=8, pady=(0, 10))

        # Humidity card
        hc = self._card(fr, row=2, title="Humidity (%)")
        self._hum_fig, self._hum_ax = _make_fig(2.8)
        self._hum_canvas = FigureCanvasTkAgg(self._hum_fig, master=hc)
        self._hum_canvas.draw()
        self._hum_canvas.get_tk_widget().pack(fill="x", padx=8, pady=(0, 10))

        # CO₂ card
        cc = self._card(fr, row=3, title="CO₂ Concentration (%)")
        self._co2_fig, self._co2_ax = _make_fig(2.8)
        self._co2_canvas = FigureCanvasTkAgg(self._co2_fig, master=cc)
        self._co2_canvas.draw()
        self._co2_canvas.get_tk_widget().pack(fill="x", padx=8, pady=(0, 10))

        return fr

    # ── Sheet 2: UV Light ─────────────────────────────────────────────────────
    def _build_uv_sheet(self) -> ctk.CTkFrame:
        fr = ctk.CTkFrame(self._scroll, fg_color="transparent")
        fr.grid_columnconfigure(0, weight=1)
        fr.grid_columnconfigure(1, weight=0)

        ctk.CTkLabel(fr, text="UV Light",
                     font=ctk.CTkFont(size=18, weight="bold")).grid(
            row=0, column=0, columnspan=2, padx=20, pady=(18, 6), sticky="w")

        # Irradiance plot (left, expands)
        plot_card = ctk.CTkFrame(fr, corner_radius=12)
        plot_card.grid(row=1, column=0, padx=(20, 6), pady=(0, 24), sticky="nsew")
        ctk.CTkLabel(plot_card, text="Irradiance (W/m²)",
                     font=ctk.CTkFont(size=12, weight="bold")).pack(
            anchor="w", padx=12, pady=(10, 4))
        self._uv_fig, self._uv_ax = _make_fig(3.6)
        self._uv_canvas = FigureCanvasTkAgg(self._uv_fig, master=plot_card)
        self._uv_canvas.draw()
        self._uv_canvas.get_tk_widget().pack(fill="x", padx=8, pady=(0, 10))

        # Info box (right, fixed width)
        info = ctk.CTkFrame(fr, width=196, corner_radius=12)
        info.grid(row=1, column=1, padx=(0, 20), pady=(0, 24), sticky="ns")
        info.pack_propagate(False)

        ctk.CTkLabel(info, text="UVI Index",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(pady=(28, 4))
        self._uvi_val = ctk.CTkLabel(
            info, text="—",
            font=ctk.CTkFont(size=50, weight="bold"),
            text_color=CLR["uv"],
        )
        self._uvi_val.pack()
        ctk.CTkLabel(info, text="unitless",
                     font=ctk.CTkFont(size=10),
                     text_color=("gray55", "gray55")).pack(pady=(2, 24))

        ctk.CTkFrame(info, height=1, fg_color=("gray72", "gray28")).pack(fill="x", padx=14)

        ctk.CTkLabel(info, text="Irradiance",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(pady=(22, 4))
        self._irr_val = ctk.CTkLabel(
            info, text="—",
            font=ctk.CTkFont(size=20, weight="bold"),
            text_color=("gray25", "gray85"),
        )
        self._irr_val.pack()
        ctk.CTkLabel(info, text="W/m²",
                     font=ctk.CTkFont(size=10),
                     text_color=("gray55", "gray55")).pack(pady=(2, 24))

        return fr

    # ── Sheet 3: Microfluidics (placeholder) ──────────────────────────────────
    def _build_micro_sheet(self) -> ctk.CTkFrame:
        fr = ctk.CTkFrame(self._scroll, fg_color="transparent")
        fr.grid_columnconfigure(0, weight=1)
        fr.grid_rowconfigure(0, weight=1)

        inner = ctk.CTkFrame(fr, fg_color="transparent")
        inner.grid(row=0, column=0, pady=120)
        ctk.CTkLabel(inner, text="Microfluidics Control",
                     font=ctk.CTkFont(size=28, weight="bold"),
                     text_color=("gray48", "gray58")).pack()
        ctk.CTkLabel(inner, text="Coming soon",
                     font=ctk.CTkFont(size=13),
                     text_color=("gray62", "gray48")).pack(pady=10)
        return fr

    # ── card factory ──────────────────────────────────────────────────────────
    @staticmethod
    def _card(parent: ctk.CTkFrame, row: int, title: str = "") -> ctk.CTkFrame:
        card = ctk.CTkFrame(parent, corner_radius=12)
        card.grid(row=row, column=0, padx=20, pady=(0, 10), sticky="ew")
        if title:
            ctk.CTkLabel(card, text=title,
                         font=ctk.CTkFont(size=12, weight="bold")).pack(
                anchor="w", padx=12, pady=(10, 4))
        return card

    # ── tab navigation ────────────────────────────────────────────────────────
    def _show(self, index: int) -> None:
        for i, sh in enumerate(self._sheets):
            if i == index:
                sh.grid(row=0, column=0, sticky="nsew")
            else:
                sh.grid_remove()
        self._active = index
        self._highlight_tab()

    def _highlight_tab(self) -> None:
        for i, b in enumerate(self._tab_btns):
            b.configure(
                fg_color="#1f6aa5" if i == self._active else ("gray78", "gray23")
            )

    # ── connection ────────────────────────────────────────────────────────────
    def _toggle_connect(self) -> None:
        if self._ws.connected:
            self._ws.disconnect()
        else:
            self._ws.connect(WS_URL)

    def _set_connected(self, ok: bool) -> None:
        if ok:
            self._dot.configure(text_color="#26de81")
            self._conn_lbl.configure(text="Connected")
            self._conn_btn.configure(text="Disconnect")
        else:
            self._dot.configure(text_color="gray42")
            self._conn_lbl.configure(text="Disconnected")
            self._conn_btn.configure(text="Connect")

    # ── data ingestion (called from WS thread) ────────────────────────────────
    def _ingest(self, data: dict) -> None:
        with self._lock:
            self._elapsed += 0.5
            self._t.append(self._elapsed)
            self._bufs["temp1"].append(float(data.get("temp1", 0)))
            self._bufs["temp2"].append(float(data.get("temp2", 0)))
            self._bufs["hum1"].append( float(data.get("hum1",  0)))
            self._bufs["hum2"].append( float(data.get("hum2",  0)))
            self._bufs["co2"].append(  float(data.get("co2",   0)))
            self._bufs["uv_irr"].append(float(data.get("uvW",     0)))
            self._bufs["uv_idx"].append(float(data.get("uvIndex", 0)))

    # ── redraw loop ───────────────────────────────────────────────────────────
    def _tick(self) -> None:
        self._redraw()
        self.after(REDRAW_MS, self._tick)

    def _redraw(self) -> None:
        with self._lock:
            t       = list(self._t)
            temp1   = list(self._bufs["temp1"])
            temp2   = list(self._bufs["temp2"])
            hum1    = list(self._bufs["hum1"])
            hum2    = list(self._bufs["hum2"])
            co2     = list(self._bufs["co2"])
            uv_irr  = list(self._bufs["uv_irr"])
            uv_idx  = list(self._bufs["uv_idx"])

        pal = _pal()

        if self._active == 0:
            _draw_series(self._temp_ax, t,
                         [(temp1, "Sensor 1", CLR["temp1"]),
                          (temp2, "Sensor 2", CLR["temp2"])],
                         pal, ylabel="°C")
            self._temp_fig.patch.set_facecolor(pal["bg"])
            self._temp_canvas.draw_idle()

            _draw_series(self._hum_ax, t,
                         [(hum1, "Sensor 1", CLR["hum1"]),
                          (hum2, "Sensor 2", CLR["hum2"])],
                         pal, ylabel="%")
            self._hum_fig.patch.set_facecolor(pal["bg"])
            self._hum_canvas.draw_idle()

            _draw_series(self._co2_ax, t,
                         [(co2, "CO₂", CLR["co2"])],
                         pal, ylabel="%")
            self._co2_fig.patch.set_facecolor(pal["bg"])
            self._co2_canvas.draw_idle()

        elif self._active == 1:
            _draw_series(self._uv_ax, t,
                         [(uv_irr, "Irradiance", CLR["uv"])],
                         pal, ylabel="W/m²")
            self._uv_fig.patch.set_facecolor(pal["bg"])
            self._uv_canvas.draw_idle()

            last_idx = uv_idx[-1] if uv_idx else 0.0
            last_irr = uv_irr[-1] if uv_irr else 0.0
            self._uvi_val.configure(text=f"{last_idx:.2f}")
            self._irr_val.configure(text=f"{last_irr:.4f}")

    # ── theme toggle ──────────────────────────────────────────────────────────
    def _toggle_theme(self) -> None:
        new = "Light" if ctk.get_appearance_mode() == "Dark" else "Dark"
        ctk.set_appearance_mode(new)
        self._theme_btn.configure(text="☀" if new == "Dark" else "🌙")

    # ── send dialog ───────────────────────────────────────────────────────────
    def _open_send(self) -> None:
        SendDialog(self, self._ws)


# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    App().mainloop()
