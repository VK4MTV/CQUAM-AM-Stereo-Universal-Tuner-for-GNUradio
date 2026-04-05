#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// MainWindow.h  –  Qt6 GUI for the AM Stereo Receiver
// ─────────────────────────────────────────────────────────────────────────────
#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <memory>

#include "../sdr/SDRDeviceManager.h"
#include "../dsp/DSPPipeline.h"
#include "../audio/AudioOutput.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onFreqChanged(int hz);
    void onAudioBwChanged(int hz);
    void onNotchModeChanged(int id);
    void onNotchFreqVChanged(int hz);
    void onMonoModeToggled(bool stereo);
    void onRfGainChanged(int dB);
    void onAudioGainChanged(int percent);
    void updateStatusIndicators();
    void onDeviceError(const QString& msg);

private:
    void buildUI();
    void buildControls(QWidget* parent);
    void startRadio();
    void stopRadio();

    // ── SDR + DSP + Audio ─────────────────────────────────────────────────────
    SDRDeviceManager   sdr_;
    DSPPipeline        dsp_;
    AudioOutput        audio_;

    // ── GUI widgets ───────────────────────────────────────────────────────────
    QSpinBox*      freqSpinBox_       = nullptr;  // frequency in kHz
    QSlider*       freqSlider_        = nullptr;
    QSlider*       audioBwSlider_     = nullptr;
    QLabel*        audioBwLabel_      = nullptr;
    QSlider*       rfGainSlider_      = nullptr;  // SDR RF gain (0–49 dB)
    QLabel*        rfGainLabel_       = nullptr;
    QSlider*       audioGainSlider_   = nullptr;  // audio output gain (1–400 %)
    QLabel*        audioGainLabel_    = nullptr;
    QButtonGroup*  notchGroup_        = nullptr;
    QSlider*       notchFreqVSlider_  = nullptr;
    QLabel*        notchFreqVLabel_   = nullptr;
    QCheckBox*     stereoCheckBox_    = nullptr;

    // LED indicators (drawn as coloured QLabels)
    QLabel*        pilotLed_          = nullptr;
    QLabel*        lockLed_           = nullptr;
    QLabel*        deviceLabel_       = nullptr;

    // Status polling timer
    QTimer*        statusTimer_       = nullptr;

    // Current notch mode (0=5k,1=9k,2=10k,3=variable)
    int            notchMode_         = 2;
    int            notchFreqV_        = 5000;
};
