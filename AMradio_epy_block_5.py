import numpy as np
from gnuradio import gr
import math
import pmt
from scipy import signal

class cquam_decoder(gr.sync_block):
    def __init__(self, sample_rate=120000, notch_freq=5000, notch_q=50.0, mono_mode=False):
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
        
        # Initialize filter coefficients and memory (Manual Biquad)
        self._update_notch_coeffs()
        self.w1_l, self.w2_l = 0.0, 0.0
        self.w1_r, self.w2_r = 0.0, 0.0
        
        # Goertzel Parameters for 25Hz
        self.g_coeff = 2.0 * math.cos(2.0 * math.pi * 25.0 / self.fs)
        self.g_s1, self.g_s2 = 0.0, 0.0
        
        self.message_port_register_in(pmt.intern("notch_freq"))
        self.set_msg_handler(pmt.intern("notch_freq"), self.handle_notch_freq)

    def _update_gains(self):
        T = 1.0 / self.fs
        denom = 1.0 + 2.0 * self.zeta * self.omegaN * T + (self.omegaN * T)**2
        self.alpha = (2.0 * self.zeta * self.omegaN * T) / denom
        self.beta = ((self.omegaN * T)**2) / denom

    def _update_notch_coeffs(self):
        b, a = signal.iirnotch(self.notch_freq, self.notch_q, self.fs)
        # Store as simple floats for the manual loop
        self.nb0, self.nb1, self.nb2 = b[0], b[1], b[2]
        self.na1, self.na2 = a[1], a[2]
        # Reset memory to stop pops when changing freq
        self.w1_l, self.w2_l = 0.0, 0.0
        self.w1_r, self.w2_r = 0.0, 0.0

    def set_notch_freq(self, notch_freq):
        if self.notch_freq != notch_freq:
            self.notch_freq = notch_freq
            self._update_notch_coeffs()

    def set_mono_mode(self, mode):
        self.mono_mode = bool(mode)

    def handle_notch_freq(self, msg):
        try:
            freq_val = pmt.to_long(msg)
            self.set_notch_freq(freq_val)
        except: pass

    def get_lock_status(self):
        return self.lock_level

    def get_pilot_status(self):
        if self.lock_level > 0.8 and self.pilot_mag > 0.001: 
            return 1.0
        return 0.0

    def work(self, input_items, output_items):
        inp = input_items[0]
        L_out, R_out = output_items[0], output_items[1]
        n = len(inp)

        # Local variables for max speed (Avoids 'self.' lookups in the loop)
        phz, omega2, cos_gamma = self.phz, self.omega2, self.cos_gamma
        s1, s2 = self.g_s1, self.g_s2
        g_coeff = self.g_coeff
        
        # Notch filter local states
        nb0, nb1, nb2, na1, na2 = self.nb0, self.nb1, self.nb2, self.na1, self.na2
        w1_l, w2_l = self.w1_l, self.w2_l
        w1_r, w2_r = self.w1_r, self.w2_r

        for i in range(n):
            # 1. Demodulation
            vco = math.cos(phz) - 1j * math.sin(phz)
            bb = inp[i] * vco
            I, Q = bb.real, bb.imag
            env = math.sqrt(I*I + Q*Q) + 1e-9
            
            # 2. PLL & C-QUAM Sync
            det = Q / env 
            omega2 += self.beta * det
            phz = (phz + self.alpha * det + omega2) % (2.0 * math.pi)
            cos_gamma += 0.005 * ((I / env) - cos_gamma)
            
            LpR = (env * cos_gamma) - 1.0
            LmR = Q / cos_gamma

            # 3. Pilot Detection (Goertzel)
            s0 = LmR + g_coeff * s1 - s2
            s2, s1 = s1, s0

            # 4. Carrier Lock Monitoring
            self.lock_level += 0.001 * (max(0, I/env) - self.lock_level)
            
            # 5. Extract Raw L/R
            raw_l = 0.5 * (LpR + LmR)
            raw_r = 0.5 * (LpR - LmR)

            # 6. Manual Notch Filter (Direct Form II Biquad)
            # Left
            wn0_l = raw_l - na1*w1_l - na2*w2_l
            L_out[i] = nb0*wn0_l + nb1*w1_l + nb2*w2_l
            w2_l, w1_l = w1_l, wn0_l
            
            # Right
            wn0_r = raw_r - na1*w1_r - na2*w2_r
            R_out[i] = nb0*wn0_r + nb1*w1_r + nb2*w2_r
            w2_r, w1_r = w1_r, wn0_r

        # Save all states back to class
        self.phz, self.omega2, self.cos_gamma = phz, omega2, cos_gamma
        self.g_s1, self.g_s2 = s1, s2
        self.w1_l, self.w2_l = w1_l, w2_l
        self.w1_r, self.w2_r = w1_r, w2_r

        # Final Pilot Magnitude for the block
        power = s1*s1 + s2*s2 - s1*s2*g_coeff
        self.pilot_mag = 0.9 * self.pilot_mag + 0.1 * (math.sqrt(max(0, power)) / n)

        if self.mono_mode:
            mono = 0.5 * (L_out + R_out)
            L_out[:], R_out[:] = mono, mono

        return n
