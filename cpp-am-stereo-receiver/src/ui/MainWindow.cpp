// ─────────────────────────────────────────────────────────────────────────────
// MainWindow.cpp  –  Qt6 GUI for the AM Stereo Receiver
// ─────────────────────────────────────────────────────────────────────────────
#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFont>
#include <QFrame>
#include <QSizePolicy>
#include <iostream>

// ── LED helper ────────────────────────────────────────────────────────────────
static QLabel* makeLed(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel(text, parent);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setFixedSize(120, 28);
    lbl->setStyleSheet("border-radius: 8px;"
                       "background-color: #333333;"
                       "color: #888888;"
                       "font-weight: bold;"
                       "border: 1px solid #555555;");
    return lbl;
}

static void setLedState(QLabel* lbl, bool active, const QString& onColor, const QString& offColor = "#333333")
{
    if (active) {
        lbl->setStyleSheet(QString(
            "border-radius: 8px;"
            "background-color: %1;"
            "color: white;"
            "font-weight: bold;"
            "border: 1px solid #888888;").arg(onColor));
    } else {
        lbl->setStyleSheet(QString(
            "border-radius: 8px;"
            "background-color: %1;"
            "color: #888888;"
            "font-weight: bold;"
            "border: 1px solid #555555;").arg(offColor));
    }
}

// ── Constructor ───────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("AM Stereo Receiver – C-QUAM");
    setMinimumWidth(480);

    buildUI();

    // Status update timer (100 ms)
    statusTimer_ = new QTimer(this);
    connect(statusTimer_, &QTimer::timeout, this, &MainWindow::updateStatusIndicators);
    statusTimer_->start(100);

    startRadio();
}

MainWindow::~MainWindow()
{
    stopRadio();
}

// ── Build UI ──────────────────────────────────────────────────────────────────
void MainWindow::buildUI()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // ── Title ─────────────────────────────────────────────────────────────────
    {
        auto* title = new QLabel("🎙 AM Stereo Receiver (C-QUAM)", this);
        QFont f = title->font();
        f.setPointSize(14);
        f.setBold(true);
        title->setFont(f);
        title->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(title);
    }

    // ── Device label ─────────────────────────────────────────────────────────
    deviceLabel_ = new QLabel("Detecting SDR device...", this);
    deviceLabel_->setAlignment(Qt::AlignCenter);
    deviceLabel_->setStyleSheet("color: #aaaaaa; font-style: italic;");
    mainLayout->addWidget(deviceLabel_);

    // ── Frequency ─────────────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Frequency (kHz)", this);
        auto* lay = new QVBoxLayout(grp);

        freqSpinBox_ = new QSpinBox(this);
        freqSpinBox_->setRange(100, 30000);
        freqSpinBox_->setValue(7390);
        freqSpinBox_->setSuffix(" kHz");
        freqSpinBox_->setFixedHeight(32);

        freqSlider_ = new QSlider(Qt::Horizontal, this);
        freqSlider_->setRange(100, 30000);
        freqSlider_->setValue(7390);

        connect(freqSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
                freqSlider_, &QSlider::setValue);
        connect(freqSlider_, &QSlider::valueChanged,
                freqSpinBox_, &QSpinBox::setValue);
        connect(freqSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MainWindow::onFreqChanged);

        lay->addWidget(freqSpinBox_);
        lay->addWidget(freqSlider_);
        mainLayout->addWidget(grp);
    }

    // ── Stereo / Mono ─────────────────────────────────────────────────────────
    {
        stereoCheckBox_ = new QCheckBox("Stereo Mode (uncheck for Mono)", this);
        stereoCheckBox_->setChecked(true);
        connect(stereoCheckBox_, &QCheckBox::toggled,
                this, &MainWindow::onMonoModeToggled);
        mainLayout->addWidget(stereoCheckBox_);
    }

    // ── Notch filter ─────────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Notch Filter", this);
        auto* lay = new QHBoxLayout(grp);

        notchGroup_ = new QButtonGroup(this);
        const QStringList labels = { "5 kHz (SW)", "9 kHz (Global)", "10 kHz (USA)", "Variable" };
        for (int i = 0; i < labels.size(); ++i) {
            auto* rb = new QRadioButton(labels[i], this);
            notchGroup_->addButton(rb, i);
            lay->addWidget(rb);
            if (i == 2) rb->setChecked(true);  // default 10 kHz (USA)
        }
        connect(notchGroup_, &QButtonGroup::idClicked,
                this, &MainWindow::onNotchModeChanged);
        mainLayout->addWidget(grp);
    }

    // ── Variable notch slider ─────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Variable Notch Frequency", this);
        auto* lay = new QHBoxLayout(grp);

        notchFreqVSlider_ = new QSlider(Qt::Horizontal, this);
        notchFreqVSlider_->setRange(2000, 15000);
        notchFreqVSlider_->setValue(5000);

        notchFreqVLabel_ = new QLabel("5000 Hz", this);
        notchFreqVLabel_->setMinimumWidth(80);

        connect(notchFreqVSlider_, &QSlider::valueChanged, this, [this](int v) {
            notchFreqVLabel_->setText(QString::number(v) + " Hz");
            onNotchFreqVChanged(v);
        });

        lay->addWidget(notchFreqVSlider_);
        lay->addWidget(notchFreqVLabel_);
        mainLayout->addWidget(grp);
    }

    // ── Audio bandwidth ───────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Audio Bandwidth", this);
        auto* lay = new QHBoxLayout(grp);

        audioBwSlider_ = new QSlider(Qt::Horizontal, this);
        audioBwSlider_->setRange(2500, 15000);
        audioBwSlider_->setValue(10000);

        audioBwLabel_ = new QLabel("10000 Hz", this);
        audioBwLabel_->setMinimumWidth(80);

        connect(audioBwSlider_, &QSlider::valueChanged, this, [this](int v) {
            audioBwLabel_->setText(QString::number(v) + " Hz");
            onAudioBwChanged(v);
        });

        lay->addWidget(audioBwSlider_);
        lay->addWidget(audioBwLabel_);
        mainLayout->addWidget(grp);
    }

    // ── Status indicators ─────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Status", this);
        auto* lay = new QHBoxLayout(grp);
        lay->setAlignment(Qt::AlignCenter);

        pilotLed_ = makeLed("Stereo Pilot", this);
        lockLed_  = makeLed("Carrier Lock", this);

        lay->addWidget(pilotLed_);
        lay->addWidget(lockLed_);
        mainLayout->addWidget(grp);
    }

    mainLayout->addStretch();
}

// ── startRadio ────────────────────────────────────────────────────────────────
void MainWindow::startRadio()
{
    // Open audio first
    if (!audio_.open()) {
        QMessageBox::warning(this, "Audio Error",
                             "Could not open audio output device.\n"
                             "DSP will run but audio will be silent.");
    }

    // Wire DSP → audio
    dsp_.setAudioCallback([this](const float* data, std::size_t frames) {
        audio_.push(data, frames);
    });

    // Auto-detect SDR
    if (!sdr_.autoOpen()) {
        deviceLabel_->setText("⚠ No SDR device found – check connection and drivers");
        deviceLabel_->setStyleSheet("color: #ff6666; font-weight: bold;");
        QMessageBox::warning(this, "SDR Not Found",
                             "No SoapySDR device was detected.\n\n"
                             "Please:\n"
                             "  1. Connect your RTL-SDR (or other SoapySDR device)\n"
                             "  2. Ensure drivers are installed\n"
                             "  3. Restart the application");
        return;
    }

    const auto& dev = sdr_.activeDevice();
    deviceLabel_->setText(QString("Device: %1").arg(QString::fromStdString(dev.label)));
    deviceLabel_->setStyleSheet("color: #88ff88;");

    // Apply initial frequency (subtract 100 kHz offset so the SDR tunes to
    // target - 100 kHz; the DSP FreqXlator then shifts the signal back up by
    // +100 kHz, placing the station of interest at baseband and keeping DC
    // leakage 100 kHz away from the wanted signal)
    const double freqHz = freqSpinBox_->value() * 1'000.0;
    sdr_.setFrequency(freqHz - 100'000.0);

    // Apply fixed RF gain (calibrated value; not exposed to the user)
    sdr_.setGain(30.0);

    // Start SDR stream → feed DSP pipeline
    sdr_.startStream([this](const std::complex<float>* buf, std::size_t count) {
        dsp_.processIQ(buf, count);
    });
}

void MainWindow::stopRadio()
{
    sdr_.stopStream();
    sdr_.close();
    audio_.close();
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void MainWindow::onFreqChanged(int kHz)
{
    // RTL-SDR direct sampling: tune 100 kHz below the displayed frequency so
    // that the DSP FreqXlator (+100 kHz) brings the wanted signal to baseband
    // while keeping DC leakage well clear of the wanted signal.
    const double freqHz = static_cast<double>(kHz) * 1'000.0;
    sdr_.setFrequency(freqHz - 100'000.0);
}

void MainWindow::onAudioBwChanged(int hz)
{
    dsp_.setAudioBandwidth(static_cast<double>(hz));
}

void MainWindow::onNotchModeChanged(int id)
{
    notchMode_ = id;
    static const int presets[] = { 5000, 9000, 10000, 0 };
    if (id < 3) {
        dsp_.setNotchFreq(presets[id]);
    } else {
        // Variable – use current slider value
        dsp_.setNotchFreq(static_cast<double>(notchFreqV_));
    }
}

void MainWindow::onNotchFreqVChanged(int hz)
{
    notchFreqV_ = hz;
    if (notchMode_ == 3)
        dsp_.setNotchFreq(static_cast<double>(hz));
}

void MainWindow::onMonoModeToggled(bool stereo)
{
    dsp_.setMonoMode(!stereo);
}

// ── Status update (100 ms timer) ─────────────────────────────────────────────
void MainWindow::updateStatusIndicators()
{
    if (!sdr_.isOpen()) return;

    const bool pilot = dsp_.pilotDetected();
    const bool lock  = dsp_.carrierLocked();

    setLedState(pilotLed_, pilot, "#2299ff");          // blue when pilot detected
    setLedState(lockLed_,  lock,  "#22cc44", "#cc2222"); // green/red for lock
}

void MainWindow::onDeviceError(const QString& msg)
{
    QMessageBox::critical(this, "Device Error", msg);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    stopRadio();
    event->accept();
}
