#ifndef MUSICTTTEXTDOWNLOADTHREAD_H
#define MUSICTTTEXTDOWNLOADTHREAD_H

/* =================================================
 * This file is part of the TTK Music Player project
 * Copyright (c) 2014 - 2016 Greedysky Studio
 * All rights reserved!
 * Redistribution and use of the source code or any derivative
 * works are strictly forbiden.
   =================================================*/

#include "musicdownloadthreadabstract.h"

/*! @brief The class of downloading the type of ttop txt.
 * @author Greedysky <greedysky@163.com>
 */
class MUSIC_NETWORK_EXPORT MusicTTTextDownLoadThread : public MusicDownLoadThreadAbstract
{
    Q_OBJECT
public:
    MusicTTTextDownLoadThread(const QString &url, const QString &save,
                            Download_Type type, QObject *parent = 0);
    /*!
     * Object contsructor provide download URL\ save local path and download type.
     */

    static QString getClassName();
    /*!
     * Get class object name.
     */
    virtual void startToDownload() override;
    /*!
     * Start to download data.
     */

public Q_SLOTS:
    virtual void downLoadFinished() override;
    /*!
     * Download data from net finished.
     */

};
#endif // MUSICTTTEXTDOWNLOADTHREAD_H
