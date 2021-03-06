#include "musicdownloadquerywythread.h"
#include "musicnumberdefine.h"
#include "musicutils.h"
#include "musictime.h"

#ifdef MUSIC_GREATER_NEW
#   include <QJsonArray>
#   include <QJsonObject>
#   include <QJsonValue>
#   include <QJsonParseError>
#else
#   ///QJson import
#   include "qjson/parser.h"
#endif
#include <QNetworkRequest>
#include <QNetworkAccessManager>

MusicDownLoadQueryWYThread::MusicDownLoadQueryWYThread(QObject *parent)
    : MusicDownLoadQueryThreadAbstract(parent)
{
    m_index = 0;
}

QString MusicDownLoadQueryWYThread::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownLoadQueryWYThread::startSearchSong(QueryType type, const QString &text)
{
    m_searchText = text.trimmed();
    m_currentType = type;

    if(m_reply)
    {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(MY_SEARCH_URL));
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Origin", MY_BASE_URL);
    request.setRawHeader("Referer", MY_BASE_URL);
#ifndef QT_NO_SSL
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
#endif
    QNetworkReply *reply = m_manager->post(request, MY_SEARCH_QUERY_URL.arg(text).arg(0).toUtf8());
    connect(reply, SIGNAL(finished()), SLOT(downLoadFinished()) );
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicDownLoadQueryWYThread::downLoadFinished()
{
    QNetworkReply *reply = MObject_cast(QNetworkReply*, QObject::sender());
    if(reply)
    {
        QByteArray bytes = reply->readAll();

        emit clearAllItems();
        m_musicSongInfos.clear();
        m_songIds.clear();
        m_index = 0;
#ifdef MUSIC_GREATER_NEW
        QJsonParseError jsonError;
        QJsonDocument parseDoucment = QJsonDocument::fromJson(bytes, &jsonError);
        ///Put the data into Json
        if( jsonError.error != QJsonParseError::NoError ||
            !parseDoucment.isObject())
        {
            return;
        }

        QJsonObject jsonObject = parseDoucment.object();
        if(jsonObject.contains("code") && jsonObject.value("code").toInt() == 200)
        {
            jsonObject = jsonObject.value("result").toObject();
            if(jsonObject.contains("songs"))
            {
                QJsonArray songsArray = jsonObject.take("songs").toArray();
                foreach(const QJsonValue &value, songsArray)
                {
                    if(!value.isObject())
                    {
                       continue;
                    }
                    QJsonObject object = value.toObject();
                    if(m_currentType != MovieQuery)
                    {
                        m_songIds << QString::number(object.value("id").toVariant().toLongLong());
                    }
                    else
                    {
                        m_songIds << QString::number(object.value("mvid").toVariant().toLongLong());
                    }
                    m_songIds.remove("0");
                }
            }
        }
#else
        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("code") && value["code"].toInt() == 200)
            {
                value = value["result"].toMap();
                QVariantList datas = value["songs"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    if(m_currentType != MovieQuery)
                    {
                        m_songIds << QString::number(value["id"].toLongLong());
                    }
                    else
                    {
                        m_songIds << QString::number(value["mvid"].toLongLong());
                    }
                    m_songIds.remove("0");
                }
            }
        }
#endif
        (m_currentType != MovieQuery) ? startSongListQuery() : startMVListQuery();
    }
}

void MusicDownLoadQueryWYThread::songListFinished()
{
    QNetworkReply *reply = MObject_cast(QNetworkReply*, QObject::sender());
    if(reply)
    {
        QByteArray bytes = reply->readAll();
#ifdef MUSIC_GREATER_NEW
        QJsonParseError jsonError;
        QJsonDocument parseDoucment = QJsonDocument::fromJson(bytes, &jsonError);
        ///Put the data into Json
        if( jsonError.error != QJsonParseError::NoError ||
            !parseDoucment.isObject())
        {
            emit downLoadDataChanged(QString());
            return;
        }

        QJsonObject jsonObject = parseDoucment.object();
        if(jsonObject.contains("code") && jsonObject.value("code").toInt() == 200 &&
           jsonObject.contains("songs"))
        {
            QJsonArray songsArray = jsonObject.take("songs").toArray();
            foreach(const QJsonValue &songsValue, songsArray)
            {
                if(!songsValue.isObject())
                {
                   continue;
                }

                QJsonObject object = songsValue.toObject();
                MusicObject::MusicSongInfomation info;
                info.m_songName = object.value("name").toString();
                info.m_timeLength = MusicTime::msecTime2LabelJustified(object.value("duration").toInt());
                info.m_lrcUrl = MY_SONG_LRC_URL.arg(object.value("id").toInt());

                QJsonObject albumObject = object.value("album").toObject();
                info.m_smallPicUrl = albumObject.value("picUrl").toString();

                QJsonArray artistsArray = object.value("artists").toArray();
                foreach(const QJsonValue &artistValue, artistsArray)
                {
                    if(!artistValue.isObject())
                    {
                       continue;
                    }
                    QJsonObject artistObject = artistValue.toObject();
                    info.m_singerName = artistObject.value("name").toString();
                }

                if(m_queryAllRecords)
                {
                    readFromMusicSongAttribute(&info, object.value("lMusic").toVariant().toMap(), MB_32);
                    readFromMusicSongAttribute(&info, object.value("bMusic").toVariant().toMap(), MB_128);
                    readFromMusicSongAttribute(&info, object.value("mMusic").toVariant().toMap(), MB_192);
                    readFromMusicSongAttribute(&info, object.value("hMusic").toVariant().toMap(), MB_320);
                }
                else
                {
                    if(m_searchQuality == tr("ST"))
                        readFromMusicSongAttribute(&info, object.value("lMusic").toVariant().toMap(), MB_32);
                    if(m_searchQuality == tr("SD"))
                        readFromMusicSongAttribute(&info, object.value("bMusic").toVariant().toMap(), MB_128);
                    else if(m_searchQuality == tr("HD"))
                        readFromMusicSongAttribute(&info, object.value("mMusic").toVariant().toMap(), MB_192);
                    else if(m_searchQuality == tr("SQ"))
                        readFromMusicSongAttribute(&info, object.value("hMusic").toVariant().toMap(), MB_320);
                }

                if(info.m_songAttrs.isEmpty())
                {
                    continue;
                }

                emit createSearchedItems(info.m_songName, info.m_singerName, info.m_timeLength);
                m_musicSongInfos << info;

                if( ++m_index >= m_songIds.count())
                {
                    emit downLoadDataChanged(QString());
                }
            }
            if(m_musicSongInfos.count() == 0)
            {
                emit downLoadDataChanged(QString());
            }
        }
#else
        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("code") && value.contains("songs") && value["code"].toInt() == 200)
            {
                QVariantList datas = value["songs"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    MusicObject::MusicSongInfomation info;
                    info.m_songName = value["name"].toString();
                    info.m_timeLength = MusicTime::msecTime2LabelJustified(value["duration"].toInt());
                    info.m_lrcUrl = MY_SONG_LRC_URL.arg(value["id"].toInt());

                    QVariantMap albumObject = value["album"].toMap();
                    info.m_smallPicUrl = albumObject["picUrl"].toString();

                    QVariantList artistsArray = value["artists"].toList();
                    foreach(const QVariant &artistValue, artistsArray)
                    {
                        if(artistValue.isNull())
                        {
                            continue;
                        }
                        QVariantMap artistMap = artistValue.toMap();
                        info.m_singerName = artistMap["name"].toString();
                    }

                    if(m_queryAllRecords)
                    {
                        readFromMusicSongAttribute(&info, value["lMusic"].toMap(), MB_32);
                        readFromMusicSongAttribute(&info, value["bMusic"].toMap(), MB_128);
                        readFromMusicSongAttribute(&info, value["mMusic"].toMap(), MB_192);
                        readFromMusicSongAttribute(&info, value["hMusic"].toMap(), MB_320);
                    }
                    else
                    {
                        if(m_searchQuality == tr("ST"))
                            readFromMusicSongAttribute(&info, value["lMusic"].toMap(), MB_32);
                        if(m_searchQuality == tr("SD"))
                            readFromMusicSongAttribute(&info, value["bMusic"].toMap(), MB_128);
                        else if(m_searchQuality == tr("HD"))
                            readFromMusicSongAttribute(&info, value["mMusic"].toMap(), MB_192);
                        else if(m_searchQuality == tr("SQ"))
                            readFromMusicSongAttribute(&info, value["hMusic"].toMap(), MB_320);
                    }

                    if(info.m_songAttrs.isEmpty())
                    {
                        continue;
                    }

                    emit createSearchedItems(info.m_songName, info.m_singerName, info.m_timeLength);
                    m_musicSongInfos << info;

                    if( ++m_index >= m_songIds.count())
                    {
                        emit downLoadDataChanged(QString());
                    }
                }
                if(m_musicSongInfos.count() == 0)
                {
                    emit downLoadDataChanged(QString());
                }
            }
        }
#endif
    }
}

void MusicDownLoadQueryWYThread::mvListFinished()
{
    QNetworkReply *reply = MObject_cast(QNetworkReply*, QObject::sender());
    if(reply)
    {
        QByteArray bytes = reply->readAll();
#ifdef MUSIC_GREATER_NEW
        QJsonParseError jsonError;
        QJsonDocument parseDoucment = QJsonDocument::fromJson(bytes, &jsonError);
        ///Put the data into Json
        if( jsonError.error != QJsonParseError::NoError || !parseDoucment.isObject())
        {
            emit downLoadDataChanged(QString());
            return;
        }

        QJsonObject object = parseDoucment.object();
        if(object.contains("code") && object.value("code").toInt() == 200)
        {
            object = object.take("data").toObject();
            MusicObject::MusicSongInfomation info;
            info.m_songName = object.value("name").toString();
            info.m_singerName = object.value("artistName").toString();
            info.m_timeLength = MusicTime::msecTime2LabelJustified(object.value("duration").toInt());

            QJsonObject brsObject = object.take("brs").toObject();
            foreach(const QString &key, brsObject.keys())
            {
                MusicObject::MusicSongAttribute attr;
                if(key == "480")
                    attr.m_bitrate = MB_500;
                else if(key == "720")
                    attr.m_bitrate = MB_750;
                else if(key == "1080")
                    attr.m_bitrate = MB_1000;
                else
                    attr.m_bitrate = key.toInt();
                attr.m_url = brsObject.value(key).toString();
                attr.m_format = attr.m_url.right(3);
                attr.m_size = QString();
                info.m_songAttrs.append(attr);
            }

            if(info.m_songAttrs.isEmpty())
            {
                return;
            }

            emit createSearchedItems(info.m_songName, info.m_singerName, info.m_timeLength);
            m_musicSongInfos << info;

            if( ++m_index >= m_songIds.count() || m_musicSongInfos.count() == 0)
            {
                emit downLoadDataChanged(QString());
            }
        }
#else
        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("code") && value["code"].toInt() == 200)
            {
                value = value["data"].toMap();
                MusicObject::MusicSongInfomation info;
                info.m_songName = value["name"].toString();
                info.m_singerName = value["artistName"].toString();
                info.m_timeLength = MusicTime::msecTime2LabelJustified(value["duration"].toInt());

                value = value["brs"].toMap();
                foreach(const QString &key, value.keys())
                {
                    MusicObject::MusicSongAttribute attr;
                    if(key == "480")
                        attr.m_bitrate = MB_500;
                    else if(key == "720")
                        attr.m_bitrate = MB_750;
                    else if(key == "1080")
                        attr.m_bitrate = MB_1000;
                    else
                        attr.m_bitrate = key.toInt();
                    attr.m_url = value[key].toString();
                    attr.m_format = attr.m_url.right(3);
                    attr.m_size = QString();
                    info.m_songAttrs.append(attr);
                }

                if(info.m_songAttrs.isEmpty())
                {
                    return;
                }

                emit createSearchedItems(info.m_songName, info.m_singerName, info.m_timeLength);
                m_musicSongInfos << info;

                if( ++m_index >= m_songIds.count() || m_musicSongInfos.count() == 0)
                {
                    emit downLoadDataChanged(QString());
                }
            }
        }
#endif
    }
}

void MusicDownLoadQueryWYThread::startSongListQuery()
{
    foreach(const QString &id, m_songIds)
    {
        QNetworkRequest request;
        request.setUrl(QUrl(MY_SONG_URL.arg(id)));
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        request.setRawHeader("Origin", MY_BASE_URL);
        request.setRawHeader("Referer", MY_BASE_URL);
    #ifndef QT_NO_SSL
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    #endif
        QNetworkReply *reply = m_manager->get(request);
        connect(reply, SIGNAL(finished()), SLOT(songListFinished()) );
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)) );
    }
}

void MusicDownLoadQueryWYThread::startMVListQuery()
{
    foreach(const QString &id, m_songIds)
    {
        QNetworkRequest request;
        request.setUrl(QUrl(MY_SONG_MV_URL.arg(id)));
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        request.setRawHeader("Origin", MY_BASE_URL);
        request.setRawHeader("Referer", MY_BASE_URL);
    #ifndef QT_NO_SSL
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    #endif
        QNetworkReply *reply = m_manager->get(request);
        connect(reply, SIGNAL(finished()), SLOT(mvListFinished()) );
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)) );
    }
}

void MusicDownLoadQueryWYThread::readFromMusicSongAttribute(MusicObject::MusicSongInfomation *info,
                                                            const QVariantMap &key, int bitrate)
{
    MusicObject::MusicSongAttribute attr;
    qlonglong dfsId = key.value("dfsId").toLongLong();
    attr.m_bitrate = bitrate;
    attr.m_format = key.value("extension").toString();
    attr.m_size = MusicUtils::UNumber::size2Label(key.value("size").toInt());
    attr.m_url = MY_SONG_PATH_URL.arg(encryptedId(dfsId)).arg(dfsId);
    info->m_songAttrs.append(attr);
}

QString MusicDownLoadQueryWYThread::encryptedId(qlonglong id)
{
    return encryptedId(QString::number(id));
}

QString MusicDownLoadQueryWYThread::encryptedId(const QString &string)
{
    QByteArray array1(MY_ENCRYPT_STRING);
    QByteArray array2 = string.toUtf8();
    int length = array1.length();
    for(int i=0; i<array2.length(); ++i)
    {
        array2[i] = array2[i]^array1[i%length];
    }

    QByteArray encodedData = QCryptographicHash::hash(array2, QCryptographicHash::Md5);
#if (defined MUSIC_GREATER_NEW && !defined MUSIC_NO_WINEXTRAS)
    encodedData = encodedData.toBase64(QByteArray::Base64UrlEncoding);
#else
    encodedData = encodedData.toBase64();
    encodedData.replace('+', '-');
    encodedData.replace('/', '_');
#endif
    return encodedData;
}
