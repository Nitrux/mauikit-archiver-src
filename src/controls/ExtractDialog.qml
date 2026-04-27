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
    readonly property string destinationPath: control.ensureDirectoryUrl(control.destination).toString()
    readonly property string targetDirectoryPath: destinationPath.length > 0 ? destinationPath.replace(/\/$/, "") + "/" + _directoryNameField.text.trim() : ""
    readonly property bool dirExists: targetDirectoryPath.length > 0 && FB.FM.fileExists(targetDirectoryPath)

    textEntry.visible: false

    title: i18n("Extract")
    message: i18n("Extract this archive into a new folder.")

    onOpened:
    {
        _directoryNameField.text = control.suggestedDirName
        _directoryNameField.forceActiveFocus()
        _directoryNameField.selectAll()
    }

    onAccepted:
    {
        const directoryName = _directoryNameField.text.trim()

        if(!control.validate(directoryName))
            return

        Arc.StaticArchive.extract(control.fileUrl, control.ensureDirectoryUrl(control.destination), directoryName)
        close()
    }

    onRejected: close()
    onDestinationChanged: control.updateExistenceAlert()

    Maui.TextField
    {
        id: _directoryNameField
        Layout.fillWidth: true
        Maui.Controls.title: i18n("Folder name")
        placeholderText: i18n("Extraction folder")
        onTextChanged: control.updateExistenceAlert()
    }

    Label
    {
        Layout.fillWidth: true
        text: i18n("Destination")
        color: Maui.Theme.textColor
    }

    RowLayout
    {
        Layout.fillWidth: true

        Maui.TextField
        {
            id: _destinationField
            Layout.fillWidth: true
            readOnly: true
            text: control.displayPath(control.destination)
        }

        Button
        {
            text: i18n("Browse")
            icon.name: "folder-open"
            onClicked: control.pickDestination()
        }
    }

    Loader
    {
        id: _dialogLoader
        active: false
        visible: false
    }

    Component
    {
        id: _destinationDialogComponent

        FB.FileDialog
        {
            mode: FB.FileDialog.Open
            singleSelection: true
            currentPath: control.destinationPath.length > 0 ? control.destinationPath : FB.FM.homePath()
            browser.settings.onlyDirs: true

            onFinished: (urls) =>
                        {
                            if(urls.length > 0)
                            {
                                control.destination = urls[0]
                            }
                        }

            onClosed:
            {
                _dialogLoader.active = false
                _dialogLoader.sourceComponent = undefined
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

    onDirExistsChanged:
    {
        control.updateExistenceAlert()
    }

    function ensureDirectoryUrl(path)
    {
        const value = path ? path.toString().trim() : ""

        if(value.startsWith("/") && !value.startsWith("//"))
            return "file://" + encodeURI(value)

        return value
    }

    function pickDestination()
    {
        _dialogLoader.active = false
        _dialogLoader.sourceComponent = _destinationDialogComponent
        _dialogLoader.active = true
        _dialogLoader.item.open()
    }

    function validate(directoryName)
    {
        if(control.destinationPath.length === 0 || !FB.FM.fileExists(control.destinationPath))
        {
            control.alert(i18n("Base location does not exist. Try a different location."), 2)
            return false
        }

        if(directoryName.length === 0)
        {
            control.alert(i18n("Folder name can not be empty."), 2)
            return false
        }

        if(control.dirExists)
        {
            control.alert(i18n("An extraction folder with the same name already exists."), 2)
            return false
        }

        return true
    }

    function updateExistenceAlert()
    {
        const directoryName = _directoryNameField.text.trim()

        if(directoryName.length === 0)
        {
            control.alert("", 0)
            return
        }

        if(control.dirExists)
        {
            control.alert(i18n("An extraction folder with the same name already exists."), 2)
            return
        }

        control.alert("", 0)
    }
}
