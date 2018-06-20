#include "apkextractiontask.h"

#include <QUrl>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <mcpelauncher/zip_extractor.h>
#include <mcpelauncher/minecraft_extract_utils.h>
#include <mcpelauncher/apkinfo.h>
#include "versionmanager.h"

ApkExtractionTask::ApkExtractionTask(QObject *parent) : QThread(parent) {
    connect(this, &QThread::started, this, &ApkExtractionTask::emitActiveChanged);
    connect(this, &QThread::finished, this, &ApkExtractionTask::emitActiveChanged);
}

bool ApkExtractionTask::setSourceUrl(const QUrl &url) {
    if (!url.isLocalFile())
        return false;
    setSource(url.toLocalFile());
    return true;
}

void ApkExtractionTask::run() {
    QTemporaryDir dir (versionManager()->getTempTemplate());
    int elfFd = -1;
    try {
        std::string path = dir.path().toStdString();

        ZipExtractor extractor (source().toStdString());
        ApkInfo apkInfo;
        {
            auto manifest = extractor.readFile("AndroidManifest.xml");
            axml::AXMLFile manifestFile (manifest.data(), manifest.size());
            axml::AXMLParser manifestParser (manifestFile);
            apkInfo = ApkInfo::fromXml(manifestParser);
        }
        qDebug() << "Apk info: versionCode=" << apkInfo.versionCode
                 << " versionName=" << QString::fromStdString(apkInfo.versionName);

        extractor.extractTo(MinecraftExtractUtils::filterMinecraftFiles(path),
                [this](size_t current, size_t max, ZipExtractor::FileHandle const&, size_t, size_t) {
            emit progress((float)  current / max);
        });

        QString targetDir = versionManager()->getDirectoryFor(apkInfo.versionName);
        qDebug() << "Moving " << dir.path() << " to " << targetDir;
        QDir(targetDir).removeRecursively();
        if (!QDir().rename(dir.path(), targetDir))
            throw std::runtime_error("rename failed");
        dir.setAutoRemove(false);
        versionManager()->addVersion(QDir(targetDir).dirName(), QString::fromStdString(apkInfo.versionName), apkInfo.versionCode);
    } catch (std::exception& e) {
        if (elfFd != -1)
            close(elfFd);
        emit error(e.what());
        return;
    }

    emit finished();
}