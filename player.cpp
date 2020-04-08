/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "stdafx.h"

#include "player.h"


#include "playercontrols.h"
#include "playlistmodel.h"
#include "histogramwidget.h"

#include <QMediaService>
#include <QMediaPlaylist>
#include <QVideoProbe>
#include <QAudioProbe>
#include <QMediaMetaData>
#include <QtWidgets>

ISpVoice *pVoice;//语音输出
HANDLE hThread1, hThread2, hThread3;



DWORD WINAPI helloFunc1(LPVOID arg)
{
	while (1)
	{
		//printf("Hello Thread1\n");
		pVoice->Speak(L"小可爱在哪里呢？快来学习！", 0, NULL);
		SuspendThread(hThread1);
	}
	return 0;
}
DWORD WINAPI helloFunc2(LPVOID arg)											//线程函数
{

	while (1)
	{
		//printf("Hello Thread2\n");
		pVoice->Speak(L"注意力集中，请不要乱张望！", 0, NULL);
		SuspendThread(hThread2);
	}
	return 0;
}
DWORD WINAPI helloFunc3(LPVOID arg)											//线程函数
{
	while (1)
	{
		//printf("Hello Thread3\n");
		pVoice->Speak(L"你疲劳了，休息一下吧！", 0, NULL);
		SuspendThread(hThread3);
	}
	return 0;
}

Player::Player(QWidget *parent)
    : QWidget(parent)
    , videoWidget(0)
    , coverLabel(0)
    , slider(0)
    , colorDialog(0)
{
	Recon = new Identify();
//! [create-objs]
    player = new QMediaPlayer(this);
    // owned by PlaylistModel
    playlist = new QMediaPlaylist();
    player->setPlaylist(playlist);
//! [create-objs]
	connect(&timer, SIGNAL(timeout()), this, SLOT(OpenIdentify()));
    connect(player, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    connect(player, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    connect(player, SIGNAL(metaDataChanged()), SLOT(metaDataChanged()));
    connect(playlist, SIGNAL(currentIndexChanged(int)), SLOT(playlistPositionChanged(int)));
    connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SLOT(statusChanged(QMediaPlayer::MediaStatus)));
    connect(player, SIGNAL(bufferStatusChanged(int)), this, SLOT(bufferingProgress(int)));
    connect(player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(videoAvailableChanged(bool)));
    connect(player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(displayErrorMessage()));
    connect(player, &QMediaPlayer::stateChanged, this, &Player::stateChanged);

//! [2]
    videoWidget = new VideoWidget(this);
    player->setVideoOutput(videoWidget);

    playlistModel = new PlaylistModel(this);
    playlistModel->setPlaylist(playlist);
//! [2]

    playlistView = new QListView(this);
    playlistView->setModel(playlistModel);
    playlistView->setCurrentIndex(playlistModel->index(playlist->currentIndex(), 0));

    connect(playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(jump(QModelIndex)));

    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, player->duration() / 1000);

    labelDuration = new QLabel(this);
    connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(seek(int)));

    labelHistogram = new QLabel(this);
    labelHistogram->setText("Histogram:");
    videoHistogram = new HistogramWidget(this);
    audioHistogram = new HistogramWidget(this);
    QHBoxLayout *histogramLayout = new QHBoxLayout;
    histogramLayout->addWidget(labelHistogram);
    histogramLayout->addWidget(videoHistogram, 1);
    histogramLayout->addWidget(audioHistogram, 2);

    videoProbe = new QVideoProbe(this);
    connect(videoProbe, SIGNAL(videoFrameProbed(QVideoFrame)), videoHistogram, SLOT(processFrame(QVideoFrame)));
    videoProbe->setSource(player);

    audioProbe = new QAudioProbe(this);
    connect(audioProbe, SIGNAL(audioBufferProbed(QAudioBuffer)), audioHistogram, SLOT(processBuffer(QAudioBuffer)));
    audioProbe->setSource(player);
	QidentifyButton = new QPushButton(tr("OpenIdentify"), this); //开起识别按钮
	QidentifyButton->setCheckable(true);
	connect(QidentifyButton, SIGNAL(clicked()), this, SLOT(opencamera()));

    QPushButton *openButton = new QPushButton(tr("Open"), this);

    connect(openButton, SIGNAL(clicked()), this, SLOT(open()));

    PlayerControls *controls = new PlayerControls(this);
    controls->setState(player->state());
    controls->setVolume(player->volume());
    controls->setMuted(controls->isMuted());

    connect(controls, SIGNAL(play()), player, SLOT(play()));
    connect(controls, SIGNAL(pause()), player, SLOT(pause()));
    connect(controls, SIGNAL(stop()), player, SLOT(stop()));
    connect(controls, SIGNAL(next()), playlist, SLOT(next()));
    connect(controls, SIGNAL(previous()), this, SLOT(previousClicked()));
    connect(controls, SIGNAL(changeVolume(int)), player, SLOT(setVolume(int)));
    connect(controls, SIGNAL(changeMuting(bool)), player, SLOT(setMuted(bool)));
    connect(controls, SIGNAL(changeRate(qreal)), player, SLOT(setPlaybackRate(qreal)));

    connect(controls, SIGNAL(stop()), videoWidget, SLOT(update()));

    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)),
            controls, SLOT(setState(QMediaPlayer::State)));
    connect(player, SIGNAL(volumeChanged(int)), controls, SLOT(setVolume(int)));
    connect(player, SIGNAL(mutedChanged(bool)), controls, SLOT(setMuted(bool)));

    fullScreenButton = new QPushButton(tr("FullScreen"), this);
    fullScreenButton->setCheckable(true);

    colorButton = new QPushButton(tr("Color Options..."), this);
    colorButton->setEnabled(false);
    connect(colorButton, SIGNAL(clicked()), this, SLOT(showColorDialog()));

    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(videoWidget, 2);
    displayLayout->addWidget(playlistView);


	textBrowser_1 = new QTextBrowser;
	connect(textBrowser_1, SIGNAL(cursorPositionChanged()), this, SLOT(on_slotAutoScroll()));
	textBrowser_1->setStyleSheet(QLatin1String("QTextBrowser{\n"
		"	border-width:1px;\n"
		"	border-style:solid;\n"
		"	border-color:rgb(0,0,0)\n"
		"}"));

	QBoxLayout *textLayout = new QHBoxLayout;
	textLayout->addWidget(textBrowser_1);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(openButton);
    controlLayout->addStretch(1);
    controlLayout->addWidget(controls);
    controlLayout->addStretch(1);
    controlLayout->addWidget(fullScreenButton);
    controlLayout->addWidget(colorButton);
	controlLayout->addWidget(QidentifyButton);//加入QBoxLayout里修改QidentifyButton位置

    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(displayLayout);
	layout->addLayout(textLayout);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(slider);
    hLayout->addWidget(labelDuration);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);
    layout->addLayout(histogramLayout);

    setLayout(layout);

    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"\
                                "Please check the media service plugins are installed."));

        controls->setEnabled(false);
        playlistView->setEnabled(false);
        openButton->setEnabled(false);
        colorButton->setEnabled(false);
        fullScreenButton->setEnabled(false);		
    }
	::CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);//获取ISpVoice接口	
	hThread1 = CreateThread(NULL, 0,helloFunc1, NULL, 0, NULL);				//创建线程
	SuspendThread(hThread1);
	hThread2 = CreateThread(NULL, 0, helloFunc2, NULL, 0, NULL);				//创建线程
	SuspendThread(hThread2);
	hThread3 = CreateThread(NULL, 0, helloFunc3, NULL, 0, NULL);				//创建线程
	SuspendThread(hThread3);

	metaDataChanged();
}


Player::~Player()
{

}


void Player::opencamera()
{
	//打开摄像头
	if (Recon->CheckCamera())
	{
		timer.stop();
		Recon->sequence_reader.Close();
		Recon->contro_Identify = false;
		textBrowser_1->insertPlainText("close the camera...\n");
		textBrowser_1->show();
		QidentifyButton->setText(tr("OpenIdentify"));
	}
	else
	{
		QidentifyButton->setText(tr("CloseIdentify"));
		textBrowser_1->insertPlainText("Opening the camera...\n");
		if (!Recon->face_model->eye_model)
		{
			textBrowser_1->insertPlainText("WARNING: no eye model found\n");
		}
		if (Recon->face_analyser->GetAUClassNames().size() == 0 && Recon->face_analyser->GetAUClassNames().size() == 0)
		{
			textBrowser_1->insertPlainText( "WARNING: no Action Unit models found\n");
		}
		if (!Recon->face_model->eye_model)
		{
			//如果眼睛模型获取失败
			textBrowser_1->insertPlainText("WARNING: no eye model defined, but outputting gaze\n");
		}
		textBrowser_1->show();
		Recon->sequence_reader.OpenWebcam(0, 640, 480, -1.0F, -1.0F, -1.0F, -1.0F);
		Recon->contro_Identify = true;
		timer.start(33);

	}
}
void Player::on_slotAutoScrol()
{

	textBrowser_1->moveCursor(QTextCursor::End);  //将接收文本框的滚动条滑到最下面

}
void Player::OpenIdentify()
{
	Recon->face_identify();
	idenyify_eye_time++;
	identify_poseup_time++;
	identify_posedown_time++;
	if (!Recon->detection_success)
	{
		identify_count++;
		if (identify_count >= 30 * 3)
		{
			//人不在一定时间 报错
			ResumeThread(hThread1);
			identify_count = 0;
		}

	}
	else
	{ 
		if (Recon->gaze_angle > 12.0 || Recon-> gaze_angle < -4.0)
		{
			Recon->count_gaze++;
			if (Recon->count_gaze >= 30 * 5)
			{
				textBrowser_1->insertPlainText("Attention, don't look around! \n");
				ResumeThread(hThread2);
				Recon->count_gaze = 0;
			}
		}
		if (Recon->pose_angle[1] > pose_threshold_right || Recon->pose_angle[1] < pose_threshold_left)
		{
			Recon->count_pose++;

			if (Recon->count_pose >= 30 * 5)
			{
				textBrowser_1->insertPlainText("Attention, don't look around \n");
				ResumeThread(hThread2);
				Recon->count_pose = 0;
			}
		}
		//y轴（向下）
		if (Recon->pose_angle[0] > pose_threshold_down || Recon->pose_angle[0] < pose_threshold_up)
		{
			Recon->count_pose++;
			double pos = Recon->pose_angle[0];
			if (Recon->count_pose >= 30 * 5)
			{
				textBrowser_1->insertPlainText("Attention, don't look around\n");
				ResumeThread(hThread2);
				Recon->count_pose = 0;
			}
			//if(Recon->pose_angle[0] > pose_threshold_down)
			//{
			//	Recon->pose_down_count++;
			//}
		}
		if (Recon->lefteye_y_value < Threshold_lefteye_y && Recon->righteye_y_value < Threshold_righteye_y)
		{

			Recon->count_eye_influent++;
		}
		if (idenyify_eye_time >= eye_one_time)//时间累积30s
		{
			idenyify_eye_time = 0;
			//如果眨眼频率超过规定值
			if (Recon->count_eye_influent > eye_frequce)
			{
				std::cout << "  You are tired, take a rest" << std::endl;
				ResumeThread(hThread3);

			}
			Recon->count_eye_influent = 0;
		}
		if (Recon->poseEstimate < up_reach_threshold)
		{
			Recon->pose_up_count++;

		}
		if (identify_poseup_time >= face_uptime_Threshold)
		{
			identify_poseup_time = 0;
			if (Recon->pose_up_count > pose_up_threshold_count)
			{

				textBrowser_1->insertPlainText(" You are tired, take a rest  \n");
				ResumeThread(hThread3);
			}
			Recon->pose_up_count = 0;
		}
		//低头
		if (Recon->poseEstimate> down_reach_threshold)
		{
			Recon->pose_down_count++;

		}
		if (identify_posedown_time  >= face_downtime_Threshold)
		{
			identify_posedown_time = 0;
			if (Recon->pose_down_count > pose_down_threshold_count)
			{
				textBrowser_1->insertPlainText("  You are tired, take a rest\n" );
				ResumeThread(hThread3);
				textBrowser_1->show();
			}
			Recon->pose_down_count = 0;
		}
		if (Recon->lip > up_lip_threshold)
		{
			textBrowser_1->insertPlainText( " You are tired, take a rest\n" );
			ResumeThread(hThread3);

		}
	}
}
bool Player::isPlayerAvailable() const
{
    return player->isAvailable();
}

void Player::open()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Files"));
    QStringList supportedMimeTypes = player->supportedMimeTypes();
    if (!supportedMimeTypes.isEmpty()) {
        supportedMimeTypes.append("audio/x-m3u"); // MP3 playlists
        fileDialog.setMimeTypeFilters(supportedMimeTypes);
    }
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
    if (fileDialog.exec() == QDialog::Accepted)
        addToPlaylist(fileDialog.selectedUrls());
}

static bool isPlaylist(const QUrl &url) // Check for ".m3u" playlists.
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists() && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

void Player::addToPlaylist(const QList<QUrl> urls)
{
    foreach (const QUrl &url, urls) {
        if (isPlaylist(url))
            playlist->load(url);
        else
            playlist->addMedia(url);
    }
}

void Player::durationChanged(qint64 duration)
{
    this->duration = duration/1000;
    slider->setMaximum(duration / 1000);
}

void Player::positionChanged(qint64 progress)
{
    if (!slider->isSliderDown()) {
        slider->setValue(progress / 1000);
    }
    updateDurationInfo(progress / 1000);
}

void Player::metaDataChanged()
{
    if (player->isMetaDataAvailable()) {
        setTrackInfo(QString("%1 - %2")
                .arg(player->metaData(QMediaMetaData::AlbumArtist).toString())
                .arg(player->metaData(QMediaMetaData::Title).toString()));

        if (coverLabel) {
            QUrl url = player->metaData(QMediaMetaData::CoverArtUrlLarge).value<QUrl>();

            coverLabel->setPixmap(!url.isEmpty()
                    ? QPixmap(url.toString())
                    : QPixmap());
        }
    }
}

void Player::previousClicked()
{
    // Go to previous track if we are within the first 5 seconds of playback
    // Otherwise, seek to the beginning.
    if(player->position() <= 5000)
        playlist->previous();
    else
        player->setPosition(0);
}

void Player::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        playlist->setCurrentIndex(index.row());
        player->play();
    }
}

void Player::playlistPositionChanged(int currentItem)
{
    clearHistogram();
    playlistView->setCurrentIndex(playlistModel->index(currentItem, 0));
}

void Player::seek(int seconds)
{
    player->setPosition(seconds * 1000);
}

void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);

    // handle status message
    switch (status) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Media Stalled"));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void Player::stateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::StoppedState)
        clearHistogram();
}

void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia ||
        status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void Player::bufferingProgress(int progress)
{
    setStatusInfo(tr("Buffering %4%").arg(progress));
}

void Player::videoAvailableChanged(bool available)
{
    if (!available) {
        disconnect(fullScreenButton, SIGNAL(clicked(bool)),
                    videoWidget, SLOT(setFullScreen(bool)));
        disconnect(videoWidget, SIGNAL(fullScreenChanged(bool)),
                fullScreenButton, SLOT(setChecked(bool)));
        videoWidget->setFullScreen(false);
    } else {
        connect(fullScreenButton, SIGNAL(clicked(bool)),
                videoWidget, SLOT(setFullScreen(bool)));
        connect(videoWidget, SIGNAL(fullScreenChanged(bool)),
                fullScreenButton, SLOT(setChecked(bool)));

        if (fullScreenButton->isChecked())
            videoWidget->setFullScreen(true);
    }
    colorButton->setEnabled(available);
}

void Player::setTrackInfo(const QString &info)
{
    trackInfo = info;
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(trackInfo);
}

void Player::setStatusInfo(const QString &info)
{
    statusInfo = info;
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(trackInfo);
}

void Player::displayErrorMessage()
{
    setStatusInfo(player->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || duration) {
        QTime currentTime((currentInfo/3600)%60, (currentInfo/60)%60, currentInfo%60, (currentInfo*1000)%1000);
        QTime totalTime((duration/3600)%60, (duration/60)%60, duration%60, (duration*1000)%1000);
        QString format = "mm:ss";
        if (duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    labelDuration->setText(tStr);
}

void Player::DoubletoChar(double data, char *a)
{
	ostringstream oss1;
	oss1 << data;
	memset(a, 0, 50);
	strcat(a, (oss1.str()).c_str());

}

void Player::showColorDialog()
{
    if (!colorDialog) {
        QSlider *brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessSlider->setRange(-100, 100);
        brightnessSlider->setValue(videoWidget->brightness());
        connect(brightnessSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setBrightness(int)));
        connect(videoWidget, SIGNAL(brightnessChanged(int)), brightnessSlider, SLOT(setValue(int)));

        QSlider *contrastSlider = new QSlider(Qt::Horizontal);
        contrastSlider->setRange(-100, 100);
        contrastSlider->setValue(videoWidget->contrast());
        connect(contrastSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setContrast(int)));
        connect(videoWidget, SIGNAL(contrastChanged(int)), contrastSlider, SLOT(setValue(int)));

        QSlider *hueSlider = new QSlider(Qt::Horizontal);
        hueSlider->setRange(-100, 100);
        hueSlider->setValue(videoWidget->hue());
        connect(hueSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setHue(int)));
        connect(videoWidget, SIGNAL(hueChanged(int)), hueSlider, SLOT(setValue(int)));

        QSlider *saturationSlider = new QSlider(Qt::Horizontal);
        saturationSlider->setRange(-100, 100);
        saturationSlider->setValue(videoWidget->saturation());
        connect(saturationSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setSaturation(int)));
        connect(videoWidget, SIGNAL(saturationChanged(int)), saturationSlider, SLOT(setValue(int)));

        QFormLayout *layout = new QFormLayout;
        layout->addRow(tr("Brightness"), brightnessSlider);
        layout->addRow(tr("Contrast"), contrastSlider);
        layout->addRow(tr("Hue"), hueSlider);
        layout->addRow(tr("Saturation"), saturationSlider);

        QPushButton *button = new QPushButton(tr("Close"));
        layout->addRow(button);

        colorDialog = new QDialog(this);
        colorDialog->setWindowTitle(tr("Color Options"));
        colorDialog->setLayout(layout);

        connect(button, SIGNAL(clicked()), colorDialog, SLOT(close()));
    }
    colorDialog->show();
}

void Player::clearHistogram()
{
    QMetaObject::invokeMethod(videoHistogram, "processFrame", Qt::QueuedConnection, Q_ARG(QVideoFrame, QVideoFrame()));
    QMetaObject::invokeMethod(audioHistogram, "processBuffer", Qt::QueuedConnection, Q_ARG(QAudioBuffer, QAudioBuffer()));
}
