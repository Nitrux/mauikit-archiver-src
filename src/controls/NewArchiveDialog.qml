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
            _formatBox.currentIndex = 0
            _algorithmBox.currentIndex = 0
            control.resetCompressionLevel()
            control.resetEncryption()
        }
    }

    signal done(var files, string path, string name, int type)

    /**
      *@brief The preferred location where the final archive file will be saved at
      */
    property string destination : _compressor.defaultSaveDir
    property url destinationUrl

    readonly property var compressionProfiles: _compressor.availableAlgorithms
    readonly property var formatOptions: control.buildFormatOptions(control.compressionProfiles)
    readonly property bool hasAvailableAlgorithms: formatOptions.length > 0
    readonly property var formatLabels: control.formatOptions.map(function(option) { return option.label ? option.label : "" })
    readonly property var currentFormat: (_formatBox.currentIndex >= 0 && _formatBox.currentIndex < control.formatOptions.length) ? control.formatOptions[_formatBox.currentIndex] : ({})
    readonly property var algorithmOptions: control.algorithmsForFormat(control.compressionProfiles, control.currentFormat.id ? control.currentFormat.id : "")
    readonly property var algorithmLabels: control.algorithmOptions.map(function(option) { return option.methodLabel ? option.methodLabel : "" })
    readonly property var currentAlgorithm: (_algorithmBox.currentIndex >= 0 && _algorithmBox.currentIndex < control.algorithmOptions.length) ? control.algorithmOptions[_algorithmBox.currentIndex] : ({})
    readonly property string currentExtension: currentAlgorithm.extension ? currentAlgorithm.extension : ""
    readonly property bool encryptionAvailable: currentAlgorithm.encryptionEnabled === true
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
                if(!control.validateOptions())
                    return

                const ok = control.checkExistance(_archiveNameField.text, _locationField.text, control.currentExtension)

                if(!ok)
                    return

                control.done(control.urls, control.destinationUrl.toString(), _archiveNameField.text, _formatBox.currentIndex)
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
        id: _formatBox
        Layout.fillWidth: true
        enabled: control.hasAvailableAlgorithms
        model: control.formatLabels
        currentIndex: control.hasAvailableAlgorithms ? 0 : -1

        Maui.Controls.title: i18n("Archive format")
        Maui.Controls.subtitle: i18n("Choose the archive container")

        onActivated:
        {
            _algorithmBox.currentIndex = 0
            control.resetCompressionLevel()
            control.resetEncryption()
            control.checkExistance(_archiveNameField.text, control.destinationUrl, control.currentExtension)
        }
    }

    ComboBox
    {
        id: _algorithmBox
        Layout.fillWidth: true
        enabled: control.algorithmOptions.length > 0
        model: control.algorithmLabels
        currentIndex: control.algorithmOptions.length > 0 ? 0 : -1

        Maui.Controls.title: i18n("Compression algorithm")
        Maui.Controls.subtitle: i18n("Available options depend on the compression tools found in PATH")

        onActivated:
        {
            control.resetCompressionLevel()
            control.resetEncryption()
            control.checkExistance(_archiveNameField.text, control.destinationUrl, control.currentExtension)
        }
    }

    ColumnLayout
    {
        Layout.fillWidth: true
        Layout.bottomMargin: Maui.Style.space.medium
        spacing: Maui.Style.space.small
        visible: control.currentAlgorithm.levelEnabled === true
        enabled: visible

        FontMetrics
        {
            id: _levelValueMetrics
            font: _levelValueLabel.font
        }

        Maui.Controls.title: i18n("Compression level")
        Maui.Controls.subtitle: i18n("Choose how aggressively the archive should be compressed")

        RowLayout
        {
            Layout.fillWidth: true

            Label
            {
                text: i18n("Fast")
                color: Maui.Theme.textColor
            }

            Slider
            {
                id: _levelSlider
                Layout.fillWidth: true
                enabled: control.currentAlgorithm.levelEnabled === true
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
                text: i18n("Maximum")
                color: Maui.Theme.textColor
            }

            Label
            {
                id: _levelValueLabel
                Layout.preferredWidth: _levelValueMetrics.advanceWidth("00")
                text: control.compressionLevel
                color: Maui.Theme.textColor
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    Maui.SectionItem
    {
        Layout.fillWidth: true
        flat: true
        leftPadding: 0
        rightPadding: 0
        enabled: control.encryptionAvailable
        label1.text: i18n("Password protection")
        label2.text: control.encryptionAvailable
                     ? i18n("Encrypt the archive with AES-256")
                     : control.currentFormat.id === "tar"
                       ? i18n("TAR does not provide an encryption layer")
                       : i18n("AES-256 encryption requires 7-Zip")
        label2.wrapMode: Text.Wrap

        template.content: Switch
        {
            id: _passwordSwitch
            enabled: control.encryptionAvailable
            checked: false
            onToggled:
            {
                if(!checked)
                {
                    _passwordField.clear()
                    _confirmPasswordField.clear()
                }
            }
        }
    }

    Maui.PasswordField
    {
        id: _passwordField
        Layout.fillWidth: true
        visible: _passwordSwitch.checked
        enabled: visible
        Maui.Controls.title: i18n("Password")
        placeholderText: i18n("Enter a password")
    }

    Maui.PasswordField
    {
        id: _confirmPasswordField
        Layout.fillWidth: true
        visible: _passwordSwitch.checked
        enabled: visible
        Maui.Controls.title: i18n("Confirm password")
        placeholderText: i18n("Enter the password again")
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
            control.alert(i18n("Base location does not exist. Try a different location."), 2)
            return false
        }

        if(name.length === 0)
        {
            control.alert(i18n("File name cannot be empty."), 2)
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
        const password = _passwordSwitch.checked ? _passwordField.text : ""
        _compressor.compressWithOptions(control.urls, control.destinationUrl, _archiveNameField.text, control.currentAlgorithm, control.compressionLevel, password)
    }

    function clear()
    {
        _archiveNameField.clear()
        control.resetEncryption()
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

    function buildFormatOptions(profiles)
    {
        const options = []
        const seen = {}

        for(const profile of profiles)
        {
            const formatId = profile.formatId ? profile.formatId : ""
            if(formatId.length === 0 || seen[formatId])
                continue

            seen[formatId] = true
            options.push({
                id: formatId,
                label: profile.formatLabel ? profile.formatLabel : formatId,
                icon: profile.icon
            })
        }

        return options
    }

    function algorithmsForFormat(profiles, formatId)
    {
        const options = []

        for(const profile of profiles)
        {
            if(profile.formatId === formatId)
                options.push(profile)
        }

        return options
    }

    function validateOptions()
    {
        if(!control.currentAlgorithm.id)
        {
            control.alert(i18n("No compression algorithm is available for this archive format."), 2)
            return false
        }

        if(!_passwordSwitch.checked)
            return true

        if(!control.encryptionAvailable)
        {
            control.alert(i18n("AES-256 encryption is not available for this archive format."), 2)
            return false
        }

        if(_passwordField.text.length === 0)
        {
            control.alert(i18n("Password cannot be empty."), 2)
            return false
        }

        if(_passwordField.text !== _confirmPasswordField.text)
        {
            control.alert(i18n("Passwords do not match."), 2)
            return false
        }

        return true
    }

    function resetCompressionLevel()
    {
        control.compressionLevel = control.currentAlgorithm.defaultLevel !== undefined ? control.currentAlgorithm.defaultLevel : 0
    }

    function resetEncryption()
    {
        _passwordSwitch.checked = false
        _passwordField.clear()
        _confirmPasswordField.clear()
    }

}
