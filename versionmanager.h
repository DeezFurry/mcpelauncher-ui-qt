#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include <QObject>
#include <QVector>
#include <QMap>

class VersionInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString directory MEMBER directory CONSTANT)
    Q_PROPERTY(QString versionName MEMBER versionName CONSTANT)
    Q_PROPERTY(int versionCode MEMBER versionCode CONSTANT)
public:
    QString directory;
    QString versionName;
    int versionCode;

    VersionInfo() {}
    VersionInfo(VersionInfo const& v) : directory(v.directory), versionName(v.versionName), versionCode(v.versionCode) {}

    VersionInfo& operator=(VersionInfo const& v) {
        directory = v.directory;
        versionName = v.versionName;
        versionCode = v.versionCode;
        return *this;
    }
};

class VersionList : public QObject {
    Q_OBJECT
    Q_PROPERTY(int size READ size)
    Q_PROPERTY(VersionInfo latestDownloadedVersion READ latestDownloadedVersion)

private:
    QMap<int, VersionInfo>& m_versions;

public:
    VersionList(QMap<int, VersionInfo>& versions) : m_versions(versions) {}

    int size() const { return m_versions.size(); }

    VersionInfo* latestDownloadedVersion() const;

public slots:
    VersionInfo* get(int versionCode) const {
        auto it = m_versions.find(versionCode);
        if (it != m_versions.end())
            return &it.value();
        return nullptr;
    }

    bool contains(int versionCode) const { return m_versions.contains(versionCode); }

};

class VersionManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(VersionList* versions READ versionList NOTIFY versionListChanged)

private:
    QString baseDir;
    QMap<int, VersionInfo> m_versions;
    VersionList m_versionList;

    void loadVersions();
    void saveVersions();

public:
    VersionManager();

    // This is safe in a multi-thread env, because the baseDir can not be changed
    QString const& getBaseDir() const { return baseDir; }

    QString getTempTemplate();

    QString getDirectoryFor(std::string const& versionName);

    void addVersion(QString directory, QString versionName, int versionCode);

    int latestDownloadedVersion() const;

    VersionList* versionList() { return &m_versionList; }

public slots:
    QString getDirectoryFor(QString const& versionName);

    QString getDirectoryFor(VersionInfo* version);

signals:
    void versionListChanged();

};

#endif // VERSIONMANAGER_H