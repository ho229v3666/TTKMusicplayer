#include "musicwytranslationrequest.h"
#include "musicsemaphoreloop.h"
#include "musicwyqueryinterface.h"
#include "musicwyqueryrequest.h"

MusicWYTranslationRequest::MusicWYTranslationRequest(QObject *parent)
    : MusicTranslationRequest(parent)
{

}

void MusicWYTranslationRequest::startToDownload(const QString &data)
{
    Q_UNUSED(data);
    TTK_LOGGER_INFO(QString("%1 startToSearch").arg(getClassName()));
    deleteAll();

    MusicSemaphoreLoop loop;
    MusicWYQueryRequest *d = new MusicWYQueryRequest(this);
    d->setQueryAllRecords(false);
    d->setQuerySimplify(true);
    d->startToSearch(MusicAbstractQueryRequest::MusicQuery, QFileInfo(m_rawData["name"].toString()).baseName());
    connect(d, SIGNAL(downLoadDataChanged(QString)), &loop, SLOT(quit()));
    loop.exec();

    QUrl musicUrl;
    if(!d->isEmpty())
    {
        musicUrl.setUrl(MusicUtils::Algorithm::mdII(WY_SONG_LRC_URL, false).arg(d->getMusicSongInfos().first().m_songId));
    }

    QNetworkRequest request;
    request.setUrl(musicUrl);
    MusicObject::setSslConfiguration(&request);

    m_reply = m_manager->get(request);
    connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicWYTranslationRequest::downLoadFinished()
{
    if(!m_reply)
    {
        deleteAll();
        return;
    }

    if(m_reply->error() == QNetworkReply::NoError)
    {
        const QByteArray &bytes = m_reply->readAll();

        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("code") && value["code"].toInt() == 200)
            {
                value = value["tlyric"].toMap();
                Q_EMIT downLoadDataChanged(value["lyric"].toString());
            }
        }
        else
        {
            Q_EMIT downLoadDataChanged(QString());
        }
    }
    else
    {
        TTK_LOGGER_ERROR("Translation source data error");
        Q_EMIT downLoadDataChanged(QString());
    }

    deleteAll();
}
