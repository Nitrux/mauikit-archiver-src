import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.mauikit.controls as Maui
import org.mauikit.filebrowsing as FM

import org.mauikit.archiver as Arc

FM.FileListingDialog
{
    id: control

    onOpened:
    {
        _archiveNameField.forceActiveFocus()

        if(!hasAvailableAlgorithms)
        {
            control.alert(i18n("No supported compression tools were found in PATH."), 2)
        }else
        {
            control.resetCompressionLevel()
        }
    }

    signal done(var files, string path, string name, int type)

    /**
      *@brief The preferred location where the final archive file will be saved at
      */
    property string destination : _compressor.defaultSaveDir
    property url destinationUrl

    readonly property var algorithmOptions: control.buildAlgorithmOptions(_compressor.availableAlgorithms)
    readonly property bool hasAvailableAlgorithms: algorithmOptions.length > 0
    readonly property var algorithmLabels: control.algorithmOptions.map(function(option) { return option.label ? option.label : "" })
    readonly property var currentAlgorithm: (_algorithmBox.currentIndex >= 0 && _algorithmBox.currentIndex < control.algorithmOptions.length) ? control.algorithmOptions[_algorithmBox.currentIndex] : ({})
    readonly property string currentExtension: currentAlgorithm.extension ? currentAlgorithm.extension : ""
    property int compressionLevel: 0

    onDestinationChanged: destinationUrl = control.ensureDestinationUrl(destination)

    persistent: false

    message: i18np("Compress %1 file into a new archive", "Compress %1 files into a new archive", urls.length)

    actions:[
        Action
        {
            text: i18n("Cancel")
            Maui.Controls.status: Maui.Controls.Negative
            onTriggered:
            {
                control.clear()
                close()
            }
        },
        Action
        {
            text: i18n("Create")
            Maui.Controls.status: Maui.Controls.Positive
            enabled: control.hasAvailableAlgorithms
            onTriggered:
            {
                const ok = control.checkExistance(_archiveNameField.text, _locationField.text, control.currentExtension)

                if(!ok)
                {
                    control.alert(i18n("Some error occured. Maybe current user does not have permission for writing in this directory."), 2)
                    return
                }else
                {
                    control.done(control.urls, control.destinationUrl.toString(), _archiveNameField.text, _algorithmBox.currentIndex)
                    //            control.close()
                }
            }
        }
    ]

    Action
    {
        id: _compressSuccessAction
        text: i18n("OK")
    }

    Arc.Compressor
    {
        id: _compressor
        onCompressionFinished: (url, ok) =>
                               {
                                   if(ok)
                                   {
                                       control.clear()
                                       control.close()
                                   }else
                                   {
                                       return
                                   }

                                   Maui.App.rootComponent.notify("application-x-archive",
                                                                 ok ? i18n("File compressed successfully") : i18n("Failed to compress"),
                                                                 control.displayPath(url),
                                                                 [_compressSuccessAction])
                               }
    }

    Maui.TextField
    {
        id: _archiveNameField
        Layout.fillWidth: true
        Maui.Controls.title: i18n("Archive name")

        onTextChanged:
        {
            control.checkExistance(text, control.destinationUrl, control.currentExtension)
        }
    }

    Maui.TextField
    {
        id: _locationField
        Layout.fillWidth: true
        Maui.Controls.title: i18n("Destination")
        Maui.Controls.subtitle: i18n("The final location of the new archive")
        text: control.displayPath(control.destinationUrl)

        onTextChanged:
        {
            control.destinationUrl = control.ensureDestinationUrl(text)
            control.checkExistance(_archiveNameField.text, control.destinationUrl, control.currentExtension)
        }
    }

    ComboBox
    {
        id: _algorithmBox
        Layout.fillWidth: true
        enabled: control.hasAvailableAlgorithms
        model: control.algorithmLabels
        currentIndex: control.hasAvailableAlgorithms ? 0 : -1

        Maui.Controls.title: i18n("Compression algorithm")
        Maui.Controls.subtitle: i18n("Available options depend on the compressors found in PATH")

        onActivated:
        {
            control.resetCompressionLevel()
            control.checkExistance(_archiveNameField.text, control.destinationUrl, control.currentExtension)
        }
    }

    ColumnLayout
    {
        Layout.fillWidth: true
        Layout.bottomMargin: Maui.Style.space.medium
        spacing: Maui.Style.space.small

        FontMetrics
        {
            id: _levelValueMetrics
            font: _levelValueLabel.font
        }

        Maui.Controls.title: i18n("Compression level")
        Maui.Controls.subtitle: control.currentAlgorithm.levelSupported ? i18n("Choose how aggressively the archive should be compressed") : i18n("This format does not support adjustable compression levels")

        RowLayout
        {
            Layout.fillWidth: true

            Label
            {
                text: control.currentAlgorithm.levelSupported ? i18n("Low") : i18n("N/A")
                color: Maui.Theme.textColor
            }

            Slider
            {
                id: _levelSlider
                Layout.fillWidth: true
                enabled: control.currentAlgorithm.levelSupported
                from: control.currentAlgorithm.minLevel !== undefined ? control.currentAlgorithm.minLevel : 0
                to: control.currentAlgorithm.maxLevel !== undefined ? control.currentAlgorithm.maxLevel : 9
                stepSize: 1

                Binding on value
                {
                    value: control.compressionLevel
                    restoreMode: Binding.RestoreBindingOrValue
                }

                onMoved:
                {
                    control.compressionLevel = Math.round(value)
                }
            }

            Label
            {
                text: control.currentAlgorithm.levelSupported ? i18n("High") : i18n("Fixed")
                color: Maui.Theme.textColor
            }

            Label
            {
                id: _levelValueLabel
                Layout.preferredWidth: Math.max(_levelValueMetrics.advanceWidth("00"), _levelValueMetrics.advanceWidth(i18n("None")))
                text: control.currentAlgorithm.levelSupported ? control.compressionLevel : i18n("None")
                color: Maui.Theme.textColor
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    function checkExistance(name, path, extension)
    {
        if(!control.hasAvailableAlgorithms || extension.length === 0)
        {
            control.alert(i18n("No supported compression tools were found in PATH."), 2)
            return false
        }

        const destinationPath = control.ensureDestinationUrl(path)

        if(!FM.FM.fileExists(destinationPath))
        {
            control.alert(i18n("Base location does not exists. Try with a different location."), 2)
            return false
        }

        if(name.length === 0)
        {
            control.alert(i18n("File name can not be empty."), 2)
            return false
        }

        const file = control.ensureDestinationUrl(destinationPath.toString().replace(/\/$/, "") + "/" + name + extension)
        const exists = FM.FM.fileExists(file)

        if(exists)
        {
            control.alert(i18n("File already exists. Try with another name."), 1)
            return false
        }

        control.alert(i18n("Looks good"), 0)
        return true
    }

    function compress()
    {
        _compressor.compressWithOptions(control.urls, control.destinationUrl, _archiveNameField.text, control.currentAlgorithm, control.compressionLevel)
    }

    function clear()
    {
        _archiveNameField.clear()
        control.urls = []
    }

    function displayPath(path)
    {
        const value = path ? path.toString() : ""
        if(value.startsWith("file://"))
            return decodeURIComponent(value.replace(/^file:\/\//, ""))

        return value
    }

    function ensureDestinationUrl(path)
    {
        const value = path ? path.toString().trim() : ""

        if(value.startsWith("/") && !value.startsWith("//"))
            return "file://" + encodeURI(value)

        return value
    }

    function buildAlgorithmOptions(profiles)
    {
        const options = []
        const seen = {}

        for(const profile of profiles)
        {
            const profileId = profile.id ? profile.id : ""
            let algorithmId = ""
            let label = ""
            let levelSupported = true
            let minLevel = 0
            let maxLevel = 9
            let defaultLevel = 6

            if(profileId.startsWith("zip"))
            {
                algorithmId = "zip"
                label = i18n("ZIP")
                defaultLevel = 6
            }else if(profileId.startsWith("tar"))
            {
                algorithmId = "tar"
                label = i18n("TAR")
                levelSupported = false
                minLevel = 0
                maxLevel = 0
                defaultLevel = 0
            }else if(profileId.startsWith("7zip"))
            {
                algorithmId = "7zip"
                label = i18n("7ZIP")
                defaultLevel = 5
            }

            if(algorithmId.length === 0 || seen[algorithmId])
            {
                continue
            }

            seen[algorithmId] = true
            options.push({
                id: algorithmId,
                label: label,
                icon: profile.icon,
                program: profile.program,
                extension: profile.extension,
                levelSupported: levelSupported,
                minLevel: minLevel,
                maxLevel: maxLevel,
                defaultLevel: defaultLevel
            })
        }

        return options
    }

    function resetCompressionLevel()
    {
        control.compressionLevel = control.currentAlgorithm.defaultLevel !== undefined ? control.currentAlgorithm.defaultLevel : 0
    }

}
