#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: AM Stereo radio
# Author: Christopher O'Reilly
# Copyright: Tropical Radio Network
# Description: AM Radio with C-QUAM reception
# GNU Radio version: 3.10.12.0

from PyQt5 import Qt
from gnuradio import qtgui
from PyQt5 import QtCore
from PyQt5.QtCore import QObject, pyqtSlot
from gnuradio import audio
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import AMradio_epy_block_1 as epy_block_1  # embedded python block
import AMradio_epy_block_5 as epy_block_5  # embedded python block
import osmosdr
import time
import satellites.hier
import threading



class AMradio(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "AM Stereo radio", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("AM Stereo radio")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except BaseException as exc:
            print(f"Qt GUI: Could not set Icon: {str(exc)}", file=sys.stderr)
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("gnuradio/flowgraphs", "AMradio")

        try:
            geometry = self.settings.value("geometry")
            if geometry:
                self.restoreGeometry(geometry)
        except BaseException as exc:
            print(f"Qt GUI: Could not restore geometry: {str(exc)}", file=sys.stderr)
        self.flowgraph_started = threading.Event()

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 1000e3
        self.pilot_val = pilot_val = 0
        self.notch_mode = notch_mode = 2
        self.notch_freq_v = notch_freq_v = 5000
        self.mono_mode = mono_mode = 0
        self.freq = freq = 7390e3
        self.audio_bw = audio_bw = 10000
        self.Stereo_Lock = Stereo_Lock = 0

        ##################################################
        # Blocks
        ##################################################

        _mono_mode_check_box = Qt.QCheckBox("STEREO MODE")
        self._mono_mode_choices = {True: 0, False: 1}
        self._mono_mode_choices_inv = dict((v,k) for k,v in self._mono_mode_choices.items())
        self._mono_mode_callback = lambda i: Qt.QMetaObject.invokeMethod(_mono_mode_check_box, "setChecked", Qt.Q_ARG("bool", self._mono_mode_choices_inv[i]))
        self._mono_mode_callback(self.mono_mode)
        _mono_mode_check_box.stateChanged.connect(lambda i: self.set_mono_mode(self._mono_mode_choices[bool(i)]))
        self.top_layout.addWidget(_mono_mode_check_box)
        self.epy_block_5 = epy_block_5.cquam_decoder(sample_rate=120000, notch_freq=5000, notch_q=50, mono_mode=mono_mode)
        def _pilot_val_probe():
          self.flowgraph_started.wait()
          while True:

            val = self.epy_block_5.get_pilot_status()
            try:
              try:
                self.doc.add_next_tick_callback(functools.partial(self.set_pilot_val,val))
              except AttributeError:
                self.set_pilot_val(val)
            except AttributeError:
              pass
            time.sleep(1.0 / (10))
        _pilot_val_thread = threading.Thread(target=_pilot_val_probe)
        _pilot_val_thread.daemon = True
        _pilot_val_thread.start()
        # Create the options list
        self._notch_mode_options = [0, 1, 2, 3]
        # Create the labels list
        self._notch_mode_labels = ['5KHz (SW)', '9KHz (Global)', '10KHz (USA)', 'Variable']
        # Create the combo box
        # Create the radio buttons
        self._notch_mode_group_box = Qt.QGroupBox("Notch filter" + ": ")
        self._notch_mode_box = Qt.QHBoxLayout()
        class variable_chooser_button_group(Qt.QButtonGroup):
            def __init__(self, parent=None):
                Qt.QButtonGroup.__init__(self, parent)
            @pyqtSlot(int)
            def updateButtonChecked(self, button_id):
                self.button(button_id).setChecked(True)
        self._notch_mode_button_group = variable_chooser_button_group()
        self._notch_mode_group_box.setLayout(self._notch_mode_box)
        for i, _label in enumerate(self._notch_mode_labels):
            radio_button = Qt.QRadioButton(_label)
            self._notch_mode_box.addWidget(radio_button)
            self._notch_mode_button_group.addButton(radio_button, i)
        self._notch_mode_callback = lambda i: Qt.QMetaObject.invokeMethod(self._notch_mode_button_group, "updateButtonChecked", Qt.Q_ARG("int", self._notch_mode_options.index(i)))
        self._notch_mode_callback(self.notch_mode)
        self._notch_mode_button_group.buttonClicked[int].connect(
            lambda i: self.set_notch_mode(self._notch_mode_options[i]))
        self.top_layout.addWidget(self._notch_mode_group_box)
        self._notch_freq_v_range = qtgui.Range(2000, 15000, 1, 5000, 200)
        self._notch_freq_v_win = qtgui.RangeWidget(self._notch_freq_v_range, self.set_notch_freq_v, "Variable notch", "counter_slider", int, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._notch_freq_v_win)
        self._freq_msgdigctl_win = qtgui.MsgDigitalNumberControl(lbl='Frequency', min_freq_hz=500e3, max_freq_hz=30000e3, parent=self, thousands_separator=",", background_color="black", fontColor="white", var_callback=self.set_freq, outputmsgname='freq')
        self._freq_msgdigctl_win.setValue(7390e3)
        self._freq_msgdigctl_win.setReadOnly(False)
        self.freq = self._freq_msgdigctl_win

        self.top_layout.addWidget(self._freq_msgdigctl_win)
        self._audio_bw_range = qtgui.Range(2500, 15000, 1, 10000, 200)
        self._audio_bw_win = qtgui.RangeWidget(self._audio_bw_range, self.set_audio_bw, "Audio bandwidth", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._audio_bw_win)
        def _Stereo_Lock_probe():
          self.flowgraph_started.wait()
          while True:

            val = self.epy_block_5.get_lock_status()
            try:
              try:
                self.doc.add_next_tick_callback(functools.partial(self.set_Stereo_Lock,val))
              except AttributeError:
                self.set_Stereo_Lock(val)
            except AttributeError:
              pass
            time.sleep(1.0 / (10))
        _Stereo_Lock_thread = threading.Thread(target=_Stereo_Lock_probe)
        _Stereo_Lock_thread.daemon = True
        _Stereo_Lock_thread.start()
        self.satellites_rms_agc_0 = satellites.hier.rms_agc(alpha=(1e-4), reference=0.2)
        self.rtlsdr_source_0 = osmosdr.source(
            args="numchan=" + str(1) + " " + 'rtl=0,direct_samp=2'
        )
        self.rtlsdr_source_0.set_time_unknown_pps(osmosdr.time_spec_t())
        self.rtlsdr_source_0.set_sample_rate(samp_rate)
        self.rtlsdr_source_0.set_center_freq((freq+100000), 0)
        self.rtlsdr_source_0.set_freq_corr(0, 0)
        self.rtlsdr_source_0.set_dc_offset_mode(0, 0)
        self.rtlsdr_source_0.set_iq_balance_mode(0, 0)
        self.rtlsdr_source_0.set_gain_mode(True, 0)
        self.rtlsdr_source_0.set_gain(0, 0)
        self.rtlsdr_source_0.set_if_gain(0, 0)
        self.rtlsdr_source_0.set_bb_gain(1, 0)
        self.rtlsdr_source_0.set_antenna('', 0)
        self.rtlsdr_source_0.set_bandwidth(15000, 0)
        self.rational_resampler_xxx_0_0_0_0_0 = filter.rational_resampler_fff(
                interpolation=48,
                decimation=120,
                taps=[],
                fractional_bw=0)
        self.rational_resampler_xxx_0_0_0_0_0.set_min_output_buffer(4)
        self.rational_resampler_xxx_0_0_0_0_0.set_max_output_buffer(16)
        self.rational_resampler_xxx_0_0_0_0 = filter.rational_resampler_fff(
                interpolation=48,
                decimation=120,
                taps=[],
                fractional_bw=0)
        self.rational_resampler_xxx_0_0_0_0.set_min_output_buffer(4)
        self.rational_resampler_xxx_0_0_0_0.set_max_output_buffer(16)
        self.rational_resampler_xxx_0_0_0 = filter.rational_resampler_ccc(
                interpolation=120,
                decimation=1000,
                taps=[],
                fractional_bw=0)
        self.rational_resampler_xxx_0_0_0.set_min_output_buffer(4)
        self.rational_resampler_xxx_0_0_0.set_max_output_buffer(16)
        self.pilot_led = self._pilot_led_win = qtgui.GrLEDIndicator('Stereo Pilot', "blue", "black", True if pilot_val > 0.5 else False, 40, 1, 3, 1, self)
        self.pilot_led = self._pilot_led_win
        self.top_layout.addWidget(self._pilot_led_win)
        self.low_pass_filter_0 = filter.interp_fir_filter_ccf(
            1,
            firdes.low_pass(
                3,
                120000,
                audio_bw,
                2000,
                window.WIN_HAMMING,
                6.76))
        self.low_pass_filter_0.set_min_output_buffer(4)
        self.low_pass_filter_0.set_max_output_buffer(16)
        self.lock_val = self._lock_val_win = qtgui.GrLEDIndicator('Carrier Lock', "green", "red", True if Stereo_Lock > 0.7 else False, 40, 1, 2, 1, self)
        self.lock_val = self._lock_val_win
        self.top_layout.addWidget(self._lock_val_win)
        self.freq_xlating_fft_filter_ccc_0 = filter.freq_xlating_fft_filter_ccc(1, [1], (-100e3), samp_rate)
        self.freq_xlating_fft_filter_ccc_0.set_nthreads(1)
        self.freq_xlating_fft_filter_ccc_0.declare_sample_delay(0)
        self.epy_block_1 = epy_block_1.notch_frequency_controller(notch_mode=notch_mode, notch_freq_v=notch_freq_v)
        self.audio_sink_0 = audio.sink(48000, '', True)


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.epy_block_1, 'notch_freq'), (self.epy_block_5, 'notch_freq'))
        self.connect((self.epy_block_5, 0), (self.rational_resampler_xxx_0_0_0_0, 0))
        self.connect((self.epy_block_5, 1), (self.rational_resampler_xxx_0_0_0_0_0, 0))
        self.connect((self.freq_xlating_fft_filter_ccc_0, 0), (self.rational_resampler_xxx_0_0_0, 0))
        self.connect((self.low_pass_filter_0, 0), (self.epy_block_5, 0))
        self.connect((self.rational_resampler_xxx_0_0_0, 0), (self.satellites_rms_agc_0, 0))
        self.connect((self.rational_resampler_xxx_0_0_0_0, 0), (self.audio_sink_0, 0))
        self.connect((self.rational_resampler_xxx_0_0_0_0_0, 0), (self.audio_sink_0, 1))
        self.connect((self.rtlsdr_source_0, 0), (self.freq_xlating_fft_filter_ccc_0, 0))
        self.connect((self.satellites_rms_agc_0, 0), (self.low_pass_filter_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("gnuradio/flowgraphs", "AMradio")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.rtlsdr_source_0.set_sample_rate(self.samp_rate)

    def get_pilot_val(self):
        return self.pilot_val

    def set_pilot_val(self, pilot_val):
        self.pilot_val = pilot_val
        self.pilot_led.setState(True if self.pilot_val > 0.5 else False)

    def get_notch_mode(self):
        return self.notch_mode

    def set_notch_mode(self, notch_mode):
        self.notch_mode = notch_mode
        self._notch_mode_callback(self.notch_mode)
        self.epy_block_1.notch_mode = self.notch_mode
        freq_map = {0: 5000, 1: 9000, 2: 10000, 3: self.notch_freq_v}
        target_freq = freq_map.get(notch_mode, 5000)
        self.epy_block_5.set_notch_freq(target_freq)

    def get_notch_freq_v(self):
        return self.notch_freq_v

    def set_notch_freq_v(self, notch_freq_v):
        self.notch_freq_v = notch_freq_v
        self.epy_block_1.notch_freq_v = self.notch_freq_v
        self.epy_block_5.set_notch_freq(notch_freq_v)

    def get_mono_mode(self):
        return self.mono_mode

    def set_mono_mode(self, mono_mode):
        self.mono_mode = mono_mode
        self._mono_mode_callback(self.mono_mode)
        self.epy_block_5.mono_mode = self.mono_mode

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.rtlsdr_source_0.set_center_freq((self.freq+100000), 0)

    def get_audio_bw(self):
        return self.audio_bw

    def set_audio_bw(self, audio_bw):
        self.audio_bw = audio_bw
        self.low_pass_filter_0.set_taps(firdes.low_pass(3, 120000, self.audio_bw, 2000, window.WIN_HAMMING, 6.76))

    def get_Stereo_Lock(self):
        return self.Stereo_Lock

    def set_Stereo_Lock(self, Stereo_Lock):
        self.Stereo_Lock = Stereo_Lock
        self.lock_val.setState(True if self.Stereo_Lock > 0.7 else False)




def main(top_block_cls=AMradio, options=None):

    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()
    tb.flowgraph_started.set()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
