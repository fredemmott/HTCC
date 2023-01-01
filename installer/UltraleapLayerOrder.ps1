# UltraLeap's OpenXR layer needs to be later in the list than
# HTCC's, otherwise HTCC isn't able to use it. OpenXR doesn't
# have an explicit ordering constraint system - instead, it
# loads them in the order the registry keys are created.
#
# To fix this, delete the key and re-create it
$key = "HKLM:\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit"
if (-not (Test-Path $key)) {
	# huh what? We should have at least installed our own layer
	return;
}

$layers = (Get-Item $key).Property

foreach ($layer in $layers) {
	if ($layer -like "*\UltraleapHandTracking.json") {
		$value = Get-ItemPropertyValue $key -name $layer
		Remove-ItemProperty -Path $key -Name $layer -Force
		New-ItemProperty -Path $key -Name $layer -PropertyType DWord -Value $value
	}
}
