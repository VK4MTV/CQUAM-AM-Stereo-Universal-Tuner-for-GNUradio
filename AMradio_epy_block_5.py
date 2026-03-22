import numpy as np
from gnuradio import gr
import math
import pmt
from scipy import signal

class cquam_decoder(gr.sync_block):
    def __init__(self, sample_rate=120000, notch_freq=9000, notch_q=30.0, mono_mode=False):
        gr.sync_block.__init__(self,
            name="C-QUAM Field Decoder",
            in_sig=[np.complex64],
            out_sig=[np.float32, np.float32]
        )
        self.fs = float(sample_rate)
        self.notch_freq = notch_freq
        self.notch_q = notch_q
        self.mono_mode = mono_mode
        
        
        # PLL & States
        self.zeta, self.omegaN = 0.707, 100.0
        self._update_gains()
        self.phz, self.omega2, self.cos_gamma = 0.0, 0.0, 1.0
        self.lock_level, self.pilot_mag = 0.0, 0.0
        
        # Initialize filter coefficients and states
        self._update_notch_coeffs()
        self.zi_l = signal.lfilter_zi(self.b, self.a) * 0
        self.zi_r = signal.lfilter_zi(self.b, self.a) * 0
        
        # Register message port for notch frequency updates
        self.message_port_register_in(pmt.intern("notch_freq"))
        self.set_msg_handler(pmt.intern("notch_freq"), self.handle_notch_freq)

    def _update_gains(self):
        T = 1.0 / self.fs
        denom = 1.0 + 2.0 * self.zeta * self.omegaN * T + (self.omegaN * T)**2
        self.alpha = (2.0 * self.zeta * self.omegaN * T) / denom
        self.beta = ((self.omegaN * T)**2) / denom

    def _update_notch_coeffs(self):
        # Optimized Scipy notch
        self.b, self.a = signal.iirnotch(self.notch_freq, self.notch_q, self.fs)

    def set_notch_freq(self, notch_freq):
        if self.notch_freq != notch_freq:
            self.notch_freq = notch_freq
            print(f"DEBUG: Notch frequency updated to {self.notch_freq} Hz")
            self._update_notch_coeffs()

    def set_notch_q(self, notch_q):
        if self.notch_q != notch_q:
            self.notch_q = notch_q
            self._update_notch_coeffs()

    def set_mono_mode(self, mode):
        self.mono_mode = bool(mode)

    def handle_notch_freq(self, msg):
        """Handle notch frequency changes from GUI variable"""
        try:
            if isinstance(msg, pmt.pmt_object):
                freq_val = pmt.to_float(msg)
            else:
                freq_val = float(msg)
            
            if self.notch_freq != freq_val:
                self.notch_freq = freq_val
                print(f"DEBUG: Notch frequency message received: {self.notch_freq} Hz")
                self._update_notch_coeffs()
        except Exception as e:
            print(f"Error handling notch_freq message: {e}")

    def get_lock_status(self):
        return self.lock_level

    def get_pilot_status(self):
        return 1.0 if self.pilot_mag > 0.015 else 0.0

    def work(self, input_items, output_items):
        inp = input_items[0]
        L_out = output_items[0]
        R_out = output_items[1]
        n = len(inp)

        # Buffers to hold raw demodulated audio for vector filtering
        raw_l = np.zeros(n, dtype=np.float32)
        raw_r = np.zeros(n, dtype=np.float32)
        
        phz, omega2, cos_gamma = self.phz, self.omega2, self.cos_gamma
        
        # 1. PER-SAMPLE DEMODULATION & MONITORING
        for i in range(n):
            vco = math.cos(phz) - 1j * math.sin(phz)
            bb = inp[i] * vco
            I, Q = bb.real, bb.imag
            env = math.sqrt(I*I + Q*Q) + 1e-9
            
            # PLL
            det = Q / env 
            omega2 += self.beta * det
            phz = (phz + self.alpha * det + omega2) % (2.0 * math.pi)

            # C-QUAM Logic
            cos_gamma += 0.005 * ((I / env) - cos_gamma)
            LpR = (env * cos_gamma) - 1.0
            LmR = Q / cos_gamma

            # Monitor Logic (Lock & 25Hz Pilot)
            self.lock_level += 0.001 * (max(0, I/env) - self.lock_level)
            self.pilot_mag += 0.0002 * (abs(LmR) - self.pilot_mag)

            raw_l[i], raw_r[i] = 0.5*(LpR + LmR), 0.5*(LpR - LmR)

        self.phz, self.omega2, self.cos_gamma = phz, omega2, cos_gamma

        # 2. VECTORIZED NOTCH FILTERING (OUTSIDE THE LOOP)
        L_out[:], self.zi_l = signal.lfilter(self.b, self.a, raw_l, zi=self.zi_l)
        R_out[:], self.zi_r = signal.lfilter(self.b, self.a, raw_r, zi=self.zi_r)

        if self.mono_mode:
            m = 0.5 * (L_out + R_out)
            L_out[:], R_out[:] = m, m

        return n
