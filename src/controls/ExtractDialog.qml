import QtQuick
import QtCore

import QtQuick.Controls
import QtQuick.Layouts

import org.mauikit.controls as Maui
import org.mauikit.archiver as Arc
import org.mauikit.filebrowsing as FB

Maui.InputDialog
{
    id: control

    property url destination
    property url fileUrl
    property string dirName
    readonly property string suggestedDirName: control.defaultDirectoryName(control.fileUrl, control.dirName)

    textEntry.text: suggestedDirName

    title: i18n("Extract")
    message: i18n("Extract the contents of the compressed file")

    Pane
    {
        Layout.fillWidth: true

        background: Rectangle
        {
            color: Maui.Theme.alternateBackgroundColor
            radius: Maui.Style.radiusV
        }

        contentItem: Column
        {
            Label
            {
                width: parent.width
                text: control.displayPath(control.fileUrl)
                font.family: "Monospace"
                wrapMode: Text.Wrap
                background: Rectangle
                {
                    color: Maui.Theme.alternateBackgroundColor
                    radius: Maui.Style.radiusV
                }
            }

            Label
            {
                 width: parent.width
                text: "=>"
                font.family: "Monospace"
            }

            Label
            {
                 width: parent.width
                text: control.displayPath(control.destination)
                font.family: "Monospace"
                wrapMode: Text.Wrap

                background: Rectangle
                {
                    color: Maui.Theme.alternateBackgroundColor
                    radius: Maui.Style.radiusV
                }
            }
        }
    }

    function displayPath(path)
    {
        const value = path ? path.toString() : ""
        if(value.startsWith("file://"))
            return decodeURIComponent(value.replace(/^file:\/\//, ""))

        return value
    }

    function archiveName(path)
    {
        const value = control.displayPath(path)
        const parts = value.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : value
    }

    function stripArchiveSuffix(name)
    {
        const suffixes = [
            ".tar.gz",
            ".tar.bz2",
            ".tar.xz",
            ".tar.zst",
            ".tar.lz4",
            ".tar.lzma",
            ".tgz",
            ".tbz2",
            ".txz",
            ".tzst",
            ".zip",
            ".7z",
            ".rar",
            ".tar",
            ".gz",
            ".bz2",
            ".xz",
            ".zst"
        ]

        const lowerName = name.toLowerCase()

        for (const suffix of suffixes)
        {
            if(lowerName.endsWith(suffix))
                return name.slice(0, name.length - suffix.length)
        }

        const lastDot = name.lastIndexOf(".")
        return lastDot > 0 ? name.slice(0, lastDot) : name
    }

    function defaultDirectoryName(path, preferredName)
    {
        const archiveFileName = control.archiveName(path)
        const candidate = preferredName ? preferredName.trim() : ""

        if(candidate.length > 0 && candidate.toLowerCase() !== archiveFileName.toLowerCase())
            return candidate

        const strippedName = control.stripArchiveSuffix(archiveFileName)
        return strippedName.length > 0 ? strippedName : archiveFileName
    }

    onFinished: (text) =>
                {
                    Arc.StaticArchive.extract(control.fileUrl, control.destination, text.trim())
                }

    readonly property bool dirExists: FB.FM.fileExists(control.destination+"/"+control.textEntry.text)
    onDirExistsChanged:
    {
        if(dirExists)
            control.alert(i18n("A directory with the same name already exists!"), 2)
        else
        {
            control.alert(i18n("The name looks good"), 0)
        }
    }

}
