$key="HKLM:\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit"
if (-not (Test-Path $key)) {
	echo "No layers."
	return;
}

$layers=(Get-Item $key).Property
$enabledCount=0

foreach ($layer in $layers)
{
	if (-not (Test-Path "${layer}"))
	{
		Write-Host -ForegroundColor Red "${layer} does not exist"
		continue;
	}

	$name=(Get-Content "${layer}" | ConvertFrom-Json).api_layer.name
	$disabled = Get-ItemPropertyValue $key -name $layer
	if ($disabled)
	{
		Write-Host -ForegroundColor DarkRed "${name}"
	}
	else
	{
		$enabledCount++
		Write-Host -NoNewline -ForegroundColor Green "${name}"
		Write-Host " (#${enabledCount})"
	}
	Write-Host -ForegroundColor DarkGray "`t${layer}"
}
