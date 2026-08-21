#include "compressedfile.h"

#include <KTar>
#include <KZip>
#include <KAr>

#if (defined Q_OS_LINUX || defined Q_OS_FREEBSD) && !defined Q_OS_ANDROID
#include <K7Zip>
#define MAUIKIT_ARCHIVER_HAS_K7ZIP
#endif

#include <QDirIterator>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <KLocalizedString>

#include <utility>

#include <MauiKit4/FileBrowsing/fmstatic.h>

#include "temporaryfile.h"

namespace
{
QString findSevenZipExecutable()
{
    const QStringList candidates {QStringLiteral("7zz"), QStringLiteral("7z"), QStringLiteral("7za"), QStringLiteral("7zr")};

    for (const auto &candidate : candidates)
    {
        const auto executable = QStandardPaths::findExecutable(candidate);
        if (!executable.isEmpty())
            return executable;
    }

    return {};
}

QString findSevenZipZipWriterExecutable()
{
    const QStringList candidates {QStringLiteral("7zz"), QStringLiteral("7z"), QStringLiteral("7za")};

    for (const auto &candidate : candidates)
    {
        const auto executable = QStandardPaths::findExecutable(candidate);
        if (!executable.isEmpty())
            return executable;
    }

    return {};
}

QVariantMap makeCompressionAlgorithm(const QString &id,
                                     const QString &formatId,
                                     const QString &formatLabel,
                                     const QString &methodLabel,
                                     const QString &icon,
                                     const QString &program,
                                     const QString &writer,
                                     const QString &filterProgram,
                                     const QString &extension,
                                     const int defaultLevel,
                                     const int minLevel,
                                     const int maxLevel,
                                     const bool levelEnabled,
                                     const bool encryptionEnabled)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("formatId"), formatId},
        {QStringLiteral("formatLabel"), formatLabel},
        {QStringLiteral("methodLabel"), methodLabel},
        {QStringLiteral("label"), formatLabel},
        {QStringLiteral("icon"), icon},
        {QStringLiteral("program"), program},
        {QStringLiteral("writer"), writer},
        {QStringLiteral("filterProgram"), filterProgram},
        {QStringLiteral("extension"), extension},
        {QStringLiteral("defaultLevel"), defaultLevel},
        {QStringLiteral("minLevel"), minLevel},
        {QStringLiteral("maxLevel"), maxLevel},
        {QStringLiteral("levelEnabled"), levelEnabled},
        {QStringLiteral("encryptionEnabled"), encryptionEnabled}
    };
}

QVariantList buildAvailableAlgorithms()
{
    QVariantList algorithms;

    const auto sevenZipExecutable = findSevenZipExecutable();
    const auto sevenZipZipWriterExecutable = findSevenZipZipWriterExecutable();
    const auto zipExecutable = QStandardPaths::findExecutable(QStringLiteral("zip"));
    const auto zipWriter = !sevenZipZipWriterExecutable.isEmpty() ? sevenZipZipWriterExecutable : zipExecutable;
    if (!zipWriter.isEmpty())
    {
        algorithms << makeCompressionAlgorithm(QStringLiteral("zip-deflate"),
                                               QStringLiteral("zip"),
                                               i18n("ZIP"),
                                               i18n("Deflate"),
                                               QStringLiteral("application-zip"),
                                               zipWriter,
                                               sevenZipZipWriterExecutable.isEmpty() ? QStringLiteral("zip") : QStringLiteral("7zip"),
                                               QString(),
                                               QStringLiteral(".zip"),
                                               6,
                                               0,
                                               9,
                                               true,
                                               !sevenZipZipWriterExecutable.isEmpty());
    }

    const auto tarExecutable = QStandardPaths::findExecutable(QStringLiteral("tar"));
    if (!tarExecutable.isEmpty())
    {
        const auto appendTarAlgorithm = [&algorithms, &tarExecutable](const QString &id,
                                                                      const QString &label,
                                                                      const QString &filterProgram,
                                                                      const QString &extension,
                                                                      const int defaultLevel,
                                                                      const int minLevel,
                                                                      const int maxLevel,
                                                                      const bool levelEnabled)
        {
            algorithms << makeCompressionAlgorithm(id,
                                                   QStringLiteral("tar"),
                                                   i18n("TAR"),
                                                   label,
                                                   QStringLiteral("application-x-tar"),
                                                   tarExecutable,
                                                   QStringLiteral("tar"),
                                                   filterProgram,
                                                   extension,
                                                   defaultLevel,
                                                   minLevel,
                                                   maxLevel,
                                                   levelEnabled,
                                                   false);
        };

        appendTarAlgorithm(QStringLiteral("tar-none"), i18n("Uncompressed"), QString(), QStringLiteral(".tar"), 0, 0, 0, false);

        const auto gzipExecutable = QStandardPaths::findExecutable(QStringLiteral("gzip"));
        if (!gzipExecutable.isEmpty())
            appendTarAlgorithm(QStringLiteral("tar-gzip"), i18n("Gzip"), gzipExecutable, QStringLiteral(".tar.gz"), 6, 1, 9, true);

        const auto bzip2Executable = QStandardPaths::findExecutable(QStringLiteral("bzip2"));
        if (!bzip2Executable.isEmpty())
            appendTarAlgorithm(QStringLiteral("tar-bzip2"), i18n("Bzip2"), bzip2Executable, QStringLiteral(".tar.bz2"), 6, 1, 9, true);

        const auto xzExecutable = QStandardPaths::findExecutable(QStringLiteral("xz"));
        if (!xzExecutable.isEmpty())
            appendTarAlgorithm(QStringLiteral("tar-xz"), i18n("XZ"), xzExecutable, QStringLiteral(".tar.xz"), 6, 0, 9, true);

        const auto zstdExecutable = QStandardPaths::findExecutable(QStringLiteral("zstd"));
        if (!zstdExecutable.isEmpty())
            appendTarAlgorithm(QStringLiteral("tar-zstd"), i18n("Zstandard"), zstdExecutable, QStringLiteral(".tar.zst"), 3, 1, 19, true);
    }

    if (!sevenZipExecutable.isEmpty())
    {
        algorithms << makeCompressionAlgorithm(QStringLiteral("7zip-lzma2"),
                                               QStringLiteral("7zip"),
                                               i18n("7ZIP"),
                                               i18n("LZMA2"),
                                               QStringLiteral("application-x-7z-compressed"),
                                               sevenZipExecutable,
                                               QStringLiteral("7zip"),
                                               QString(),
                                               QStringLiteral(".7z"),
                                               5,
                                               0,
                                               9,
                                               true,
                                               !sevenZipZipWriterExecutable.isEmpty());
    }

    return algorithms;
}

QString archivePathForAlgorithm(const QUrl &where, const QString &fileName, const QVariantMap &algorithm)
{
    return QDir(where.toLocalFile()).filePath(fileName + algorithm.value(QStringLiteral("extension")).toString());
}

QStringList redactedArguments(const QStringList &arguments)
{
    QStringList result = arguments;

    for (auto &argument : result)
    {
        if (argument.startsWith(QStringLiteral("-p")))
            argument = QStringLiteral("-p********");
    }

    return result;
}

enum class ArchiveType
{
    Unknown,
    Tar,
    Zip,
    SevenZip
};

ArchiveType archiveTypeForUrl(const QUrl &url)
{
    if (!url.isLocalFile())
        return ArchiveType::Unknown;

    const auto mimeType = FMStatic::getMime(url);
    const QStringList tarMimeTypes {
        QStringLiteral("application/x-tar"),
        QStringLiteral("application/x-compressed-tar"),
        QStringLiteral("application/x-gzip-compressed-tar"),
        QStringLiteral("application/x-bzip-compressed-tar"),
        QStringLiteral("application/x-bzip2-compressed-tar"),
        QStringLiteral("application/x-xz-compressed-tar"),
        QStringLiteral("application/x-zstd-compressed-tar")
    };

    if (tarMimeTypes.contains(mimeType))
        return ArchiveType::Tar;

    if (mimeType == QLatin1String("application/zip")
        || mimeType == QLatin1String("application/x-zip")
        || mimeType == QLatin1String("application/x-zip-compressed"))
    {
        return ArchiveType::Zip;
    }

#ifdef MAUIKIT_ARCHIVER_HAS_K7ZIP
    if (mimeType == QLatin1String("application/x-7z-compressed"))
        return ArchiveType::SevenZip;
#endif

    return ArchiveType::Unknown;
}
}


CompressedFile::CompressedFile(QObject *parent)
    : QObject(parent)
    , m_archive(nullptr)
    , m_model(new CompressedFileModel(this))
{
}

CompressedFileModel::CompressedFileModel(CompressedFile *parent)
    : MauiList(parent)
    , m_file(parent)
{
}

CompressedFile::~CompressedFile()
{
    qDeleteAll(m_previews);
    if(m_archive)
    {
        m_archive->close();
        delete m_archive;
    }
}

const FMH::MODEL_LIST &CompressedFileModel::items() const
{
    return m_list;
}

void CompressedFileModel::setEntries(FMH::MODEL_LIST list)
{
    m_list.clear();

    Q_EMIT this->preListChanged();

    m_list = list;

    Q_EMIT this->postListChanged();
    Q_EMIT this->countChanged ();
}

QString CompressedFile::currentPath() const
{
    return m_currentPath;
}

QString CompressedFile::fileName() const
{
    return m_fileName;
}

bool CompressedFile::canGoUp() const
{
    return m_canGoUp;
}

bool CompressedFile::opened() const
{
    return m_opened;
}

void CompressedFile::refresh()
{
    openDir(m_currentPath);
}

const KArchiveFile *CompressedFile::getFile(const QString &path)
{
    if(!m_archive)
    {
        return nullptr;
    }

    if(m_archive->isOpen())
    {
        return m_archive->directory()->file(path);
    }

    return nullptr;
}

void CompressedFile::openDir(const QString &path)
{
    if(m_url.isEmpty() || path.isEmpty() || !m_archive)
    {
        return;
    }

    if(m_archive->isOpen())
    {
        auto root = m_archive->directory();
        auto entry = root->entry(path);

        if(entry)
        {
            if(entry->isDirectory())
            {
                const KArchiveDirectory* subDir = static_cast<const KArchiveDirectory*>(entry);
                setCurrentPath(path);

                FMH::MODEL_LIST list;
                for(const auto &file : subDir->entries())
                {
                    const auto e = subDir->entry(file);
                    list << FMH::MODEL{{FMH::MODEL_KEY::IS_DIR, e->isDirectory() ? "true" : "false"}, {FMH::MODEL_KEY::LABEL, e->name()}, {FMH::MODEL_KEY::ICON, e->isDirectory() ? "folder" : FMStatic::getIconName(e->name())}, {FMH::MODEL_KEY::DATE, e->date().toString()}, {FMH::MODEL_KEY::PATH, QString(path+"%1/").arg(e->name())}, {FMH::MODEL_KEY::USER, e->user()}};

                }

                m_model->setEntries(list);
            }
        }
    }
}

void CompressedFile::goUp()
{
    this->openDir(QUrl(m_currentPath).resolved(QUrl("..")).toString());
}

void CompressedFile::goToRoot()
{
    this->openDir("/");
}

void CompressedFile::close()
{
    if (!m_archive)
        return;

    m_archive->close();

    if (m_opened)
    {
        m_opened = false;
        Q_EMIT openedChanged(m_opened);
    }
}

void CompressedFile::open()
{
    if (m_archive)
    {
        m_archive->close();
        delete m_archive;
        m_archive = nullptr;
    }

    m_archive = CompressedFile::getKArchiveObject(m_url);
    const bool opened = m_archive && m_archive->open(QIODevice::ReadOnly);

    if (m_opened != opened)
    {
        m_opened = opened;
        Q_EMIT openedChanged(m_opened);
    }

    if (!opened && m_archive)
        qWarning() << "Could not open archive" << m_url << m_archive->errorString();
}

QString CompressedFile::temporaryFile(const QString &path)
{
    if(m_previews.contains(path))
    {
        return m_previews.value(path)->url();
    }

    auto preview = new TemporaryFile;
    m_previews.insert(path, preview);

    auto file = getFile(path);
    preview->setData(file->data(), file->name());
    return preview->url();
}

bool CompressedFile::addFiles(const QStringList &urls, const QString &path)
{
    if(urls.isEmpty() || path.isEmpty())
    {
        return false;
    }

    bool success = false;

    m_archive->close();
    m_archive->open(QIODevice::ReadWrite);

    for(const auto &url : urls)
    {
        success = addFile(url, path);
    }

    m_archive->close();
    m_archive->open(QIODevice::ReadOnly);
    refresh();

    return success;
}

bool CompressedFile::addFile(const QString &url, const QString &path)
{
    auto localUrl = QUrl(url).toLocalFile();
    QFileInfo file(localUrl);

    if(!file.exists())
    {
        return false;
    }

    if(file.isDir())
    {
        return m_archive->addLocalDirectory(localUrl, path+file.fileName());
    }


    if(m_archive->addLocalFile(localUrl, path+file.fileName()))
    {
        qDebug() << "Trying to insert file to archive"<< url << localUrl << path << path+file.fileName();
        return true;
    }

    return false;
}

bool CompressedFile::extractFiles(const QStringList &urls, const QString &where)
{
    Q_UNUSED(urls);
    Q_UNUSED(where);
    return false;
}

bool CompressedFile::compress(const QStringList &files, const QUrl &where, const QString &fileName, const int &compressTypeSelected)
{
    auto compressor = new Compressor();
    connect(compressor, &Compressor::compressionFinished, this, [this, compressor](QString url, bool ok)
            {
                if(ok)
                    this->setUrl(url);

                Q_EMIT compressionFinished(url, ok);
                compressor->deleteLater();
            });
    return compressor->compress(files, where, fileName, compressTypeSelected);
}

bool CompressedFile::extractFile(const QString &url, const QString &where)
{
    Q_UNUSED(url);
    Q_UNUSED(where);
    return false;
}

void CompressedFile::setCurrentPath(QString currentPath)
{
    if (m_currentPath == currentPath)
        return;

    m_currentPath = currentPath;
    Q_EMIT currentPathChanged(m_currentPath);

    m_canGoUp = m_currentPath != "/";
    Q_EMIT canGoUpChanged(m_canGoUp);
}

void CompressedFile::extract(const QUrl &where, const QString &directory)
{
    if (!m_url.isLocalFile())
        return;    

    if(!m_archive || !m_archive->isOpen())
        return;

    QString where_ = where.toLocalFile() + "/" + directory;

    const bool ok = m_archive->directory()->copyTo(where_, true);

    Q_EMIT this->extractionFinished(where.toString(), ok);
}

/*
 *
 *  CompressTypeSelected is an integer and has to be acorrding with order in Dialog.qml
 *
 */
Compressor::Compressor(QObject *parent) : QObject(parent)
    ,m_defaultSaveDir(FMStatic::DocumentsPath)
    ,m_settings(new QSettings(QStringLiteral("org.mauikit.archiver"), "", this))
    ,m_availableAlgorithms(buildAvailableAlgorithms())
{
    m_settings->beginGroup("General");
    m_defaultSaveDir = m_settings->value("DefaultSaveDir", m_defaultSaveDir).toString();
    m_settings->endGroup();
}

static std::vector<std::string> QStringList_to_VectorString(const QList<QString>& qlist) {
    std::vector<std::string> result(qlist.size());
    for (int i=0; i<qlist.size(); i++) {
        result[i] = qlist.at(i).toUtf8().data();
    }
    return result;
}

static QString commonPathForFiles(const QStringList &files)
{
    if (files.isEmpty())
        return QStringLiteral("/");

    const auto commonPathFunc = [] (const std::vector<std::string> &dirs) -> std::string
    {
        std::vector<std::string>::const_iterator vsi = dirs.begin();
        int maxCharactersCommon = vsi->length();
        std::string compareString = *vsi;

        for (vsi = dirs.begin() + 1; vsi != dirs.end(); vsi++) {
            std::pair<std::string::const_iterator, std::string::const_iterator> p =
                std::mismatch(compareString.begin(), compareString.end(), vsi->begin());
            if ((p.first - compareString.begin()) < maxCharactersCommon)
                maxCharactersCommon = p.first - compareString.begin();
        }

        std::string::size_type found = compareString.rfind('/', maxCharactersCommon);
        return compareString.substr(0, found);
    };

    const auto commonPath = QString::fromStdString(commonPathFunc(QStringList_to_VectorString(files))).remove(QStringLiteral("file://"));
    return commonPath.isEmpty() ? QStringLiteral("/") : commonPath;
}

QVariantList Compressor::availableAlgorithms() const
{
    return m_availableAlgorithms;
}

bool Compressor::compress(const QStringList &files, const QUrl &where, const QString &fileName, const int &compressTypeSelected)
{
    const QString expectedId = [compressTypeSelected]() -> QString
    {
        switch (compressTypeSelected)
        {
        case 0: return QStringLiteral("zip");
        case 1: return QStringLiteral("tar");
        case 2: return QStringLiteral("7zip");
        default: return {};
        }
    }();

    if (!expectedId.isEmpty())
    {
        for (const auto &algorithmValue : std::as_const(m_availableAlgorithms))
        {
            const auto algorithm = algorithmValue.toMap();
            if (algorithm.value(QStringLiteral("id")).toString().startsWith(expectedId))
                return compressWithOptions(files, where, fileName, algorithm, algorithm.value(QStringLiteral("defaultLevel")).toInt(), QString());
        }
    }

    QString commonPath = "";
    auto fileWriter = [&commonPath](KArchive *archive, QFile &file) -> bool
    {
        if(!archive)
            return false;

        return archive->writeFile(file.fileName().remove(commonPath, Qt::CaseSensitivity::CaseSensitive), // Mirror file path in compressed file from current directory
                                  file.readAll(),
                                  0100775,
                                  QFileInfo(file).owner(),
                                  QFileInfo(file).group(),
                                  QDateTime(),
                                  QDateTime(),
                                  QDateTime());
    };

    auto dirWriter = [&fileWriter](KArchive *archive, QDirIterator &dir) -> bool
    {
        bool ok = false;

        if(!archive)            
        {
            return ok;
        }

        while (dir.hasNext())
        {
            auto entrie = dir.next();

            if (QFileInfo(entrie).isFile())
            {
                auto file = QFile(entrie);
                if (!file.open(QIODevice::ReadOnly))
                {
                    qDebug() << "ERROR. CURRENT USER DOES NOT HAVE PEMRISSION FOR WRITE IN THE CURRENT DIRECTORY.";
                    continue;
                }

                ok = fileWriter(archive, file);
                       // WriteFile returns if the file was written or not,
                       // but this function returns if some error occurs so for this reason it is needed to toggle the value
            }
        }

        return ok;
    };

    auto url = [&where, &fileName](const int &type) -> QString
    {
        QString format;
        switch(type)
        {
        case 0: format = ".zip"; break;
        case 1: format = ".tar"; break;
        case 2: format = ".7zip"; break;
        case 3: format = ".ar"; break;
        }

        return QUrl(where.toString() + "/" + fileName + format).toLocalFile();
    };

    auto commonPathFunc = [] (const std::vector<std::string> & dirs) -> std::string
    {
        std::vector<std::string>::const_iterator vsi = dirs.begin( ) ;
        int maxCharactersCommon = vsi->length( ) ;
        std::string compareString = *vsi ;
        for ( vsi = dirs.begin( ) + 1 ; vsi != dirs.end( ) ; vsi++ ) {
            std::pair<std::string::const_iterator , std::string::const_iterator> p =
                std::mismatch( compareString.begin( ) , compareString.end( ) , vsi->begin( ) ) ;
            if (( p.first - compareString.begin( ) ) < maxCharactersCommon )
                maxCharactersCommon = p.first - compareString.begin( ) ;
        }
        std::string::size_type found = compareString.rfind( '/' , maxCharactersCommon ) ;
        return compareString.substr( 0 , found ) ;
    };

    commonPath = QString::fromStdString(commonPathFunc( QStringList_to_VectorString(files) )).remove("file://");

    qDebug() << "The Common path is << " << commonPath;

    auto openCheck = [](KArchive *archive) -> bool
    {
        if(!archive)
            return false;

        archive->open(QIODevice::ReadWrite);
        return archive->isOpen();
    };

    bool ok = false;
    const QString fileUrl = url(compressTypeSelected);

    assert(compressTypeSelected >= 0 && compressTypeSelected <= 8);

    KArchive *archive = nullptr;

    switch (compressTypeSelected)
    {
    case 0: //.ZIP
    {
        archive = new KZip(fileUrl);
        break;
    }
    case 1: // .TAR
    {
        archive = new KTar(fileUrl);
        break;
    }
    case 2: //.7ZIP
    {
#ifdef K7ZIP_H

               // TODO: KArchive no permite comprimir ficheros del mismo modo que con TAR o ZIP. Hay que hacerlo de otra forma y requiere disponer de una libreria actualizada de KArchive.
        archive = new K7Zip(fileUrl);
#endif
        break;
    }
    case 3: //.AR
    {
        // TODO: KArchive no permite comprimir ficheros del mismo modo que con TAR o ZIP. Hay que hacerlo de otra forma y requiere disponer de una libreria actualizada de KArchive.
        archive = new KAr(fileUrl);
        break;
    }
    default:
        qDebug() << "ERROR. COMPRESSED TYPE SELECTED NOT COMPATIBLE";
        break;
    }

    if(!openCheck(archive))
    {
        ok = false;
        Q_EMIT compressionFinished(QUrl::fromLocalFile(fileUrl).toString(), ok);
        return ok;
    }

    for (const auto &uri : files)
    {
        qDebug() << "@gadominguez File:fm.cpp Funcion: compress  " << QUrl(uri).toLocalFile() << " " << fileName;

        if (!QFileInfo(QUrl(uri).toLocalFile()).isDir())
        {
            auto file = QFile(QUrl(uri).toLocalFile());
            if (!file.open(QIODevice::ReadWrite))
            {
                qDebug() << "ERROR. CURRENT USER DOES NOT HAVE PEMRISSION FOR WRITE IN THE CURRENT DIRECTORY.";
                ok = false;
                continue;
            }

            ok = fileWriter(archive, file);

        } else
        {                        
            qDebug() << "Dir: " << QUrl(uri).toLocalFile();
            auto dir = QDirIterator(QUrl(uri).toLocalFile(), QDirIterator::Subdirectories);
            ok = dirWriter(archive, dir);
        }
    }

    (void)archive->close();
                            // kzip->prepareWriting("Hello00000.txt", "gabridc", "gabridc", 1024, 0100777, QDateTime(), QDateTime(), QDateTime());
                            // kzip->writeData("Hello", sizeof("Hello"));
                            // kzip->finishingWriting();

    Q_EMIT compressionFinished(QUrl::fromLocalFile(fileUrl).toString(), ok);
    return ok;
}

bool Compressor::compressWithOptions(const QStringList &files, const QUrl &where, const QString &fileName, const QVariantMap &algorithm, int level, const QString &password)
{
    const auto program = algorithm.value(QStringLiteral("program")).toString();
    const auto extension = algorithm.value(QStringLiteral("extension")).toString();
    const auto formatId = algorithm.value(QStringLiteral("formatId")).toString();
    const auto writer = algorithm.value(QStringLiteral("writer")).toString();
    const bool levelEnabled = algorithm.value(QStringLiteral("levelEnabled")).toBool();
    const bool encryptionEnabled = algorithm.value(QStringLiteral("encryptionEnabled")).toBool();

    if (files.isEmpty() || program.isEmpty() || extension.isEmpty() || fileName.isEmpty() || !where.isLocalFile()
        || writer.isEmpty() || (!password.isEmpty() && !encryptionEnabled))
    {
        Q_EMIT compressionFinished(QString(), false);
        return false;
    }

    if (levelEnabled)
    {
        const int minLevel = algorithm.value(QStringLiteral("minLevel")).toInt();
        const int maxLevel = algorithm.value(QStringLiteral("maxLevel")).toInt();
        level = qBound(minLevel, level, maxLevel);
    }

    const auto archivePath = archivePathForAlgorithm(where, fileName, algorithm);
    const auto commonPath = commonPathForFiles(files);
    const QDir commonDir(commonPath);

    QStringList relativePaths;
    relativePaths.reserve(files.size());

    for (const auto &uri : files)
    {
        const auto absolutePath = QUrl(uri).toLocalFile();
        if (absolutePath.isEmpty())
            continue;

        relativePaths << commonDir.relativeFilePath(absolutePath);
    }

    if (relativePaths.isEmpty())
    {
        Q_EMIT compressionFinished(QUrl::fromLocalFile(archivePath).toString(), false);
        return false;
    }

    QStringList arguments;
    if (writer == QLatin1String("zip"))
    {
        arguments << QStringLiteral("-r")
                  << QStringLiteral("-%1").arg(level)
                  << archivePath;
    } else if (writer == QLatin1String("tar"))
    {
        const auto filterProgram = algorithm.value(QStringLiteral("filterProgram")).toString();
        if (!filterProgram.isEmpty())
        {
            const auto filterCommand = levelEnabled
                ? QStringLiteral("%1 -%2").arg(filterProgram).arg(level)
                : filterProgram;
            arguments << QStringLiteral("--use-compress-program=%1").arg(filterCommand);
        }

        arguments << QStringLiteral("-cf") << archivePath;
    } else if (writer == QLatin1String("7zip"))
    {
        arguments << QStringLiteral("a");

        if (formatId == QLatin1String("zip"))
        {
            arguments << QStringLiteral("-tzip")
                      << QStringLiteral("-mm=Deflate");

            if (!password.isEmpty())
                arguments << QStringLiteral("-mem=AES256");
        } else if (formatId == QLatin1String("7zip"))
        {
            arguments << QStringLiteral("-t7z")
                      << QStringLiteral("-m0=LZMA2");

            if (!password.isEmpty())
                arguments << QStringLiteral("-mhe=on");
        } else
        {
            Q_EMIT compressionFinished(QUrl::fromLocalFile(archivePath).toString(), false);
            return false;
        }

        arguments << QStringLiteral("-mx=%1").arg(level);

        if (!password.isEmpty())
            arguments << QStringLiteral("-p%1").arg(password);

        arguments << archivePath;
    } else
    {
        Q_EMIT compressionFinished(QUrl::fromLocalFile(archivePath).toString(), false);
        return false;
    }

    arguments << relativePaths;

    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setWorkingDirectory(commonPath);
    process.start();

    if (!process.waitForStarted() || !process.waitForFinished(-1))
    {
        qWarning() << "Failed to run compressor" << program << redactedArguments(arguments) << process.errorString();
        Q_EMIT compressionFinished(QUrl::fromLocalFile(archivePath).toString(), false);
        return false;
    }

    const bool ok = process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0
        && QFileInfo::exists(archivePath);

    if (!ok)
        qWarning() << "Compression command failed" << program << redactedArguments(arguments) << process.exitCode() << process.readAllStandardError();

    Q_EMIT compressionFinished(QUrl::fromLocalFile(archivePath).toString(), ok);
    return ok;
}

KArchive *CompressedFile::getKArchiveObject(const QUrl &url)
{
    switch (archiveTypeForUrl(url))
    {
    case ArchiveType::Tar:
        return new KTar(url.toLocalFile());
    case ArchiveType::Zip:
        return new KZip(url.toLocalFile());
    case ArchiveType::SevenZip:
#ifdef MAUIKIT_ARCHIVER_HAS_K7ZIP
        return new K7Zip(url.toLocalFile());
#else
        return nullptr;
#endif
    case ArchiveType::Unknown:
        qWarning() << "Unsupported archive type" << url << "MIME type:" << FMStatic::getMime(url);
        return nullptr;
    }

    return nullptr;
}

void CompressedFile::setUrl(const QUrl &url)
{
    if (m_url == url)
        return;

    m_url = url;
    Q_EMIT this->urlChanged();

    if(!FMStatic::fileExists(m_url))
    {
        qWarning()<< "File does not exists and can not be opened." << url;
        return;
    }

    m_fileName = QFileInfo(m_url.toLocalFile()).baseName();
    Q_EMIT fileNameChanged(m_fileName);

    open();
    openDir(m_currentPath);
}

QUrl CompressedFile::url() const
{
    return m_url;
}

bool CompressedFile::isOpen()
{
    if(!m_archive)
        return false;

    return m_archive->isOpen();
}

CompressedFileModel *CompressedFile::model() const
{
    return m_model;
}


Q_GLOBAL_STATIC(StaticArchive, appInstance)
StaticArchive::StaticArchive(QObject *parent) : QObject(parent)
{
}

StaticArchive *StaticArchive::instance()
{
    return appInstance();
}

QString Compressor::defaultSaveDir() const
{
    return m_defaultSaveDir;
}

void Compressor::setDefaultSaveDir(QString defaultSaveDir)
{
    if (m_defaultSaveDir == defaultSaveDir)
        return;

    m_defaultSaveDir = defaultSaveDir;

    m_settings->beginGroup("General");
    m_settings->setValue("DefaultSaveDir", m_defaultSaveDir);
    m_settings->endGroup();

    Q_EMIT defaultSaveDirChanged(m_defaultSaveDir);
}

Compressor::~Compressor()
{
    m_settings->sync();
}

bool StaticArchive::extract(QUrl url, QUrl where, QString dir)
{
    CompressedFile file;
    file.setUrl(url);

    if(!file.isOpen())
        return false;

    file.extract(where, dir);
    return true;
}

bool StaticArchive::isSupported(QUrl url)
{
    return archiveTypeForUrl(url) != ArchiveType::Unknown;
}
