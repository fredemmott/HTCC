# Copyright (c) 2023 Fred Emmott <fred@fredemmott.com>
#
# SPDX-License-Identifier: MIT
$enabledCount = 0
foreach ($root in @("HKLM", "HKCU")) {
	$key = "${root}:\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit"
	Write-Host "${root}"
	if (-not (Test-Path $key)) {
		Write-Host -Foreground DarkGray "`tNo layers."
		continue;
	}

	$layers = (Get-Item $key).Property
	if ($layers.count -eq 0) {
		Write-Host -Foreground DarkGray "`tNo layers."
		continue;
	}

	foreach ($layer in $layers) {
		if (-not (Test-Path "${layer}")) {
			Write-Host -ForegroundColor Red "`t${layer} does not exist (${root})"
			continue;
		}

		$name = (Get-Content "${layer}" | ConvertFrom-Json).api_layer.name
		$disabled = Get-ItemPropertyValue $key -name $layer
		if ($disabled) {
			Write-Host -ForegroundColor DarkRed "`t${name}"
		}
		else {
			$enabledCount++
			Write-Host -NoNewline "`t (#${enabledCount})"
			Write-Host -NoNewline -ForegroundColor DarkGray " - "
			Write-Host -ForegroundColor Green "${name}"
		}
		Write-Host -ForegroundColor DarkGray "`t`t${layer}"
	}
}
