// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "TerminalSettings.h"

#include "TerminalSettings.g.cpp"

namespace winrt::TerminalApp::implementation
{
    TerminalSettings::TerminalSettings(const ::TerminalApp::CascadiaSettings& appSettings, winrt::guid profileGuid)
    {
        const auto profile = appSettings.FindProfile(profileGuid);
        THROW_HR_IF_NULL(E_INVALIDARG, profile);

        const auto globals = appSettings.GlobalSettings();
        _ApplyProfileSettings(profile, globals.GetColorSchemes());
        _ApplyGlobalSettings(globals);
    }

    // Method Description:
    // - Create a TerminalSettings object for the provided newTerminalArgs. We'll
    //   use the newTerminalArgs to look up the profile that should be used to
    //   create these TerminalSettings. Then, we'll apply settings contained in the
    //   newTerminalArgs to the profile's settings, to enable customization on top
    //   of the profile's default values.
    // Arguments:
    // - newTerminalArgs: An object that may contain a profile name or GUID to
    //   actually use. If the Profile value is not a guid, we'll treat it as a name,
    //   and attempt to look the profile up by name instead.
    //   * Additionally, we'll use other values (such as Commandline,
    //     StartingDirectory) in this object to override the settings directly from
    //     the profile.
    // Return Value:
    // - the GUID of the created profile, and a fully initialized TerminalSettings object
    std::tuple<winrt::guid, winrt::TerminalApp::TerminalSettings> TerminalSettings::BuildSettings(const ::TerminalApp::CascadiaSettings& appSettings, const winrt::TerminalApp::NewTerminalArgs& newTerminalArgs)
    {
        const winrt::guid profileGuid = appSettings.GetProfileForArgs(newTerminalArgs);
        TerminalSettings settings{ appSettings, profileGuid };

        if (newTerminalArgs)
        {
            // Override commandline, starting directory if they exist in newTerminalArgs
            if (!newTerminalArgs.Commandline().empty())
            {
                settings.Commandline(newTerminalArgs.Commandline());
            }
            if (!newTerminalArgs.StartingDirectory().empty())
            {
                settings.StartingDirectory(newTerminalArgs.StartingDirectory());
            }
            if (!newTerminalArgs.TabTitle().empty())
            {
                settings.StartingTitle(newTerminalArgs.TabTitle());
            }
        }

        return { profileGuid, settings };
    }

    // Method Description:
    // - Create a TerminalSettings from this object. Apply our settings, as well as
    //      any colors from our color scheme, if we have one.
    // Arguments:
    // - schemes: a list of schemes to look for our color scheme in, if we have one.
    // Return Value:
    // - a new TerminalSettings object with our settings in it.
    void TerminalSettings::_ApplyProfileSettings(const TerminalApp::Profile& profile, const winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, TerminalApp::ColorScheme>& schemes)
    {
        // Fill in the Terminal Setting's CoreSettings from the profile
        _HistorySize = profile.HistorySize();
        _SnapOnInput = profile.SnapOnInput();
        _AltGrAliasing = profile.AltGrAliasing();
        _CursorHeight = profile.CursorHeight();
        _CursorShape = profile.CursorShape();

        // Fill in the remaining properties from the profile
        _ProfileName = profile.Name();
        _UseAcrylic = profile.UseAcrylic();
        _TintOpacity = profile.AcrylicOpacity();

        _FontFace = profile.FontFace();
        _FontSize = profile.FontSize();
        _FontWeight = profile.FontWeight();
        _Padding = profile.Padding();

        _Commandline = profile.Commandline();

        if (!profile.StartingDirectory().empty())
        {
            _StartingDirectory = profile.GetEvaluatedStartingDirectory();
        }

        // GH#2373: Use the tabTitle as the starting title if it exists, otherwise
        // use the profile name
        _StartingTitle = !profile.TabTitle().empty() ? profile.TabTitle() : profile.Name();

        if (profile.SuppressApplicationTitle())
        {
            _SuppressApplicationTitle = profile.SuppressApplicationTitle();
        }

        if (!profile.ColorSchemeName().empty())
        {
            ApplyColorScheme(profile.ColorSchemeName(), schemes);
        }
        if (profile.Foreground())
        {
            const til::color colorRef{ profile.Foreground().Value() };
            _DefaultForeground = static_cast<uint32_t>(colorRef);
        }
        if (profile.Background())
        {
            const til::color colorRef{ profile.Background().Value() };
            _DefaultBackground = static_cast<uint32_t>(colorRef);
        }
        if (profile.SelectionBackground())
        {
            const til::color colorRef{ profile.SelectionBackground().Value() };
            _SelectionBackground = static_cast<uint32_t>(colorRef);
        }
        if (profile.CursorColor())
        {
            const til::color colorRef{ profile.CursorColor().Value() };
            _CursorColor = static_cast<uint32_t>(colorRef);
        }

        _ScrollState = profile.ScrollState();

        if (!profile.BackgroundImagePath().empty())
        {
            _BackgroundImage = profile.GetExpandedBackgroundImagePath();
        }

        _BackgroundImageOpacity = profile.BackgroundImageOpacity();
        _BackgroundImageStretchMode = profile.BackgroundImageStretchMode();

        _BackgroundImageHorizontalAlignment = profile.BackgroundImageHorizontalAlignment();
        _BackgroundImageVerticalAlignment = profile.BackgroundImageVerticalAlignment();

        _RetroTerminalEffect = profile.RetroTerminalEffect();

        _AntialiasingMode = profile.AntialiasingMode();

        if (profile.TabColor())
        {
            const til::color colorRef{ profile.TabColor().Value() };
            _TabColor = static_cast<uint32_t>(colorRef);
        }
    }

    // Method Description:
    // - Applies appropriate settings from the globals into the given TerminalSettings.
    // Arguments:
    // - settings: a TerminalSettings object to add global property values to.
    // Return Value:
    // - <none>
    void TerminalSettings::_ApplyGlobalSettings(const TerminalApp::GlobalAppSettings& globalSettings) noexcept
    {
        _InitialRows = globalSettings.InitialRows();
        _InitialCols = globalSettings.InitialCols();

        _WordDelimiters = globalSettings.WordDelimiters();
        _CopyOnSelect = globalSettings.CopyOnSelect();
        _ForceFullRepaintRendering = globalSettings.ForceFullRepaintRendering();
        _SoftwareRendering = globalSettings.SoftwareRendering();
        _ForceVTInput = globalSettings.ForceVTInput();
    }

    // Method Description:
    // - Apply our values to the given TerminalSettings object. Sets the foreground,
    //      background, and color table of the settings object.
    // Arguments:
    // - terminalSettings: the object to apply our settings to.
    // Return Value:
    // - <none>
    void TerminalSettings::_ApplyColorScheme(const TerminalApp::ColorScheme& scheme)
    {
        _DefaultForeground = til::color{ scheme.Foreground() };
        _DefaultBackground = til::color{ scheme.Background() };
        _SelectionBackground = til::color{ scheme.SelectionBackground() };
        _CursorColor = til::color{ scheme.CursorColor() };

        const auto table = scheme.Table();
        auto const tableCount = gsl::narrow_cast<int>(table.size());
        for (int i = 0; i < tableCount; i++)
        {
            SetColorTableEntry(i, til::color{ table[i] });
        }
    }

    bool TerminalSettings::ApplyColorScheme(const hstring& scheme, const Windows::Foundation::Collections::IMapView<hstring, TerminalApp::ColorScheme>& schemes)
    {
        if (const auto found{ schemes.TryLookup(scheme) })
        {
            _ApplyColorScheme(found);
            return true;
        }
        return false;
    }

    uint32_t TerminalSettings::GetColorTableEntry(int32_t index) const noexcept
    {
        return _colorTable.at(index);
    }

    void TerminalSettings::SetColorTableEntry(int32_t index, uint32_t value)
    {
        auto const colorTableCount = gsl::narrow_cast<decltype(index)>(_colorTable.size());
        THROW_HR_IF(E_INVALIDARG, index >= colorTableCount);
        _colorTable.at(index) = value;
    }
}
