using System.CommandLine;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Security.AccessControl;
using WixSharp;
using WixSharp.CommonTasks;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Win32;
using WixToolset.Dtf.WindowsInstaller;
using RegistryHive = WixSharp.RegistryHive;

[assembly: InternalsVisibleTo(assemblyName: "HTCC-Installer.aot")] // assembly name + '.aot suffix

Argument<DirectoryInfo> inputRootArg = new("INPUT_ROOT")
{
    Description = "Location of files to include in the installer",
    Arity = ArgumentArity.ExactlyOne,
};
Option<string> signingKeyArg = new("--signing-key")
{
    Description = "Code signing key ID"
};
Option<string> timestampServerArg = new("--timestamp-server")
{
    Description = "Code signing timestamp server"
};
Option<FileInfo> stampFileArg = new("--stamp-file")
{
    Description = "The full path to the produced executable will be written here on success"
};

var command = new RootCommand
{
    inputRootArg,
    signingKeyArg,
    timestampServerArg,
    stampFileArg
};
command.SetAction(parseResult => CreateInstaller(parseResult.GetValue(inputRootArg)!,
    parseResult.GetValue(signingKeyArg), parseResult.GetValue(timestampServerArg), parseResult.GetValue(stampFileArg)));
return await new CommandLineConfiguration(command).InvokeAsync(args);

async Task SetProjectVersionFromJson(ManagedProject project, DirectoryInfo inputRoot)
{
    var stream = System.IO.File.OpenRead(inputRoot.GetFiles("installer/version.json").First().FullName);
    var options = new JsonSerializerOptions
    {
        PropertyNameCaseInsensitive = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
    };

    var version = await JsonSerializer.DeserializeAsync(
        stream, JsonVersionInfoContext.Default.JsonVersionInfo);
    Debug.Assert(version != null, nameof(version) + " != null");
    var c = version.Components;
    project.Version = new Version($"{c.A}.{c.B}.{c.C}.{c.D}");
    project.OutFileName += $"-v{c.A}.{c.B}.{c.C}";
    if (!version.Tagged)
    {
        project.OutFileName += $"+{version.TweakLabel}.{c.D}";
    }

    project.AddRegKey(
        new RegKey(
            project.DefaultFeature,
            RegistryHive.LocalMachine,
            @"SOFTWARE\Fred Emmott\HTCC\Version",
            new RegValue("Semantic", version.Readable),
            new RegValue("Readable", $"v{version.Readable}"),
            new RegValue("Major", version.Components.A),
            new RegValue("Minor", version.Components.B),
            new RegValue("Build", version.Components.C),
            new RegValue("Tweak", version.Components.D),
            new RegValue("Triple", $"{version.Components.A}.{version.Components.B}.{version.Components.C}"),
            new RegValue("Quad",
                $"{version.Components.A}.{version.Components.B}.{version.Components.C}.{version.Components.D}")
        ));
}

async Task<int> CreateInstaller(DirectoryInfo inputRoot, string? signingKeyId, string? timestampServer,
    FileInfo? stampFile)
{
    if (!System.IO.File.Exists($"{inputRoot}/bin/HTCCSettings.exe"))
    {
        Console.WriteLine($"Cannot find bin/HTCCSettings.exe in INPUT_ROOT ({inputRoot.FullName})");
        return 1;
    }

    var project = CreateProject(inputRoot);
    await SetProjectVersionFromJson(project, inputRoot);

    project.ResolveWildCards();
    CreateShortcuts(inputRoot, project);

    AddGameConfigurations(inputRoot, project);

    RegisterApiLayer(inputRoot, project);

    SignProject(project, signingKeyId, timestampServer);
    BuildMsi(project, stampFile);

    return 0;
}

void SignProject(ManagedProject managedProject, string? s, string? timestampServer1)
{
    if (s != null)
    {
        managedProject.DigitalSignature = new DigitalSignature
        {
            CertificateId = s,
            CertificateStore = StoreType.sha1Hash,
            HashAlgorithm = HashAlgorithmType.sha256,
            Description = managedProject.OutFileName,
            TimeUrl = (timestampServer1 == null) ? null : new Uri(timestampServer1),
        };
        managedProject.SignAllFiles = true;
    }
    else
    {
        managedProject.OutFileName += "-UNSIGNED";
    }
}

void BuildMsi(ManagedProject managedProject, FileInfo? fileInfo)
{
    var outFile = managedProject.BuildMsi();
    if (fileInfo != null)
    {
        System.IO.File.WriteAllText(fileInfo.FullName, $"{outFile}\n");
        Console.WriteLine($"Wrote output path '{outFile}' to `{fileInfo.FullName}`");
    }
}

ManagedProject CreateProject(DirectoryInfo inputRoot)
{
    var installerResources = new Feature("Installer Resources")
    {
        IsEnabled = false
    };

    var project =
        new ManagedProject("Hand Tracked Cockpit Clicking (HTCC)",
            new Dir(@"%ProgramFiles%\HTCC",
                new Files("bin/*.*")
                {
                    Filter = name => !name.EndsWith(".pdb")
                },
                new Files(installerResources, @"installer/*.*")));

    project.GUID = Guid.Parse("2f0cd440-8d59-4572-aabe-a7b4e7ffcdcd");
    project.Platform = Platform.x64;

    project.SourceBaseDir = inputRoot.FullName;

    project.ControlPanelInfo.Manufacturer = "Fred Emmott";
    project.LicenceFile = "installer/LICENSE.rtf";

    project.ControlPanelInfo.InstallLocation = "[INSTALLDIR]";
    project.AddRegValue(new RegValue(RegistryHive.LocalMachine, @"SOFTWARE\Fred Emmott\HandTrackedCockpitClicking",
        "InstallDir",
        "[INSTALLDIR]"));

    // CPack switched from per-machine to per-user MSIs; mark this MSI as an upgrade to either, not just
    // per machine
    project.AddActions(
        new ManagedAction(MyActions.FindAllRelatedProducts)
        {
            Condition = Condition.Always,
            Sequence = Sequence.InstallExecuteSequence,
            SequenceNumber = 5,
        },
        new ManagedAction(MyActions.FindAllRelatedProducts)
        {
            Condition = Condition.Always,
            Sequence = Sequence.InstallUISequence,
            SequenceNumber = 5,
        },
        new ElevatedManagedAction(MyActions.ReorderUltraleapLayer)
        {
            When = When.After,
            Step = Step.WriteRegistryValues,
        }
    );
    return project;
}

void AddGameConfigurations(DirectoryInfo inputRoot, ManagedProject managedProject)
{
    var registryFiles = inputRoot.GetFiles("installer/*.reg");
    foreach (var regFile in registryFiles)
    {
        managedProject.AddRegValues(Tasks.ImportRegFile(regFile.FullName));
    }
}

void RegisterApiLayer(DirectoryInfo directoryInfo, ManagedProject managedProject)
{
    managedProject.AddRegValue(new RegValue(RegistryHive.LocalMachine, Constants.ApiLayersKey,
        "[INSTALLDIR]APILayer.json", 0));
}

void CreateShortcuts(DirectoryInfo directoryInfo, ManagedProject managedProject)
{
    var target = $"{directoryInfo}\\bin\\HTCCSettings.exe".PathGetFullPath();
    var file = managedProject.AllFiles.Single(f => f.Name == target);
    file.AddShortcuts(
        new FileShortcut("HTCC Settings", "%Desktop%"),
        new FileShortcut("HTCC Settings", "%ProgramMenuFolder%"));
}

class JsonVersionComponents
{
    public int A { get; set; }
    public int B { get; set; }
    public int C { get; set; }
    public int D { get; set; }
}

class JsonVersionInfo
{
    public JsonVersionComponents Components { get; set; } = new JsonVersionComponents();
    public string Readable { get; set; } = string.Empty;
    public string TweakLabel { get; set; } = string.Empty;
    public bool Stable { get; set; }
    public bool Tagged { get; set; }
}

[JsonSourceGenerationOptions(PropertyNameCaseInsensitive = true,
    PropertyNamingPolicy = JsonKnownNamingPolicy.CamelCase)]
[JsonSerializable(typeof(JsonVersionInfo))]
internal partial class JsonVersionInfoContext : JsonSerializerContext
{
}

internal class MyActions
{
    //  TODO: replace with `project.EnableUpgradingFromPerUserToPerMachine();`
    // https://github.com/oleg-shilo/wixsharp/issues/1818
    [CustomAction]
    public static ActionResult FindAllRelatedProducts(Session session)
    {
        var upgradeCode = session.QueryUpgradeCode();
        var productCode = session.QueryProperty("ProductCode");
        var packages = AppSearch.GetRelatedProducts(upgradeCode).Where(it => it != productCode).ToArray();
        if (packages.Length >= 1)
        {
            var it = packages.First();
            session.SetProperty("WIX_UPGRADE_DETECTED", it);
            session.SetProperty("MIGRATE", it);
            session.SetProperty("UPGRADINGPRODUCTCODE", it);
        }

        return ActionResult.Success;
    }

    [CustomAction]
    public static ActionResult ReorderUltraleapLayer(Session session)
    {
        try
        {
            ReorderUltraleapLayer();
            return ActionResult.Success;
        }
        catch (Exception e)
        {
            session.Log($"Exception reordering Ultraleap layer: {e.Message}");
            return ActionResult.Failure;
        }
    }

    private static void ReorderUltraleapLayer()
    {
        var key = Registry.LocalMachine.OpenSubKey(Constants.ApiLayersKey,
            RegistryKeyPermissionCheck.ReadWriteSubTree,
            RegistryRights.QueryValues | RegistryRights.SetValue);


        if (key == null)
        {
            return;
        }

        foreach (var valueName in key.GetValueNames())
        {
            if (!valueName.EndsWith("\\UltraleapHandTracking.json"))
            {
                continue;
            }

            var value = key.GetValue(valueName)!;
            // Delete and recreate to put it at the end
            key.DeleteValue(valueName);
            key.SetValue(valueName, value);
        }

        key.Close();
    }
}

internal class Constants
{
    public const string ApiLayersKey = @"SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit";
}