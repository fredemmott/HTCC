param(
  [Parameter(Position = 0,mandatory=$true)]
  [string]$inputFile,
  [Parameter(Position = 1,mandatory=$true)]
  [string]$outputFile
)

Add-Type -AssemblyName System.Windows.Forms

$Rtb = New-Object -TypeName System.Windows.Forms.RichTextBox
$Rtb.MultiLine = $true;
$Rtb.Lines = (Get-Content $inputFile -Encoding utf8) -split "`n"
Set-Content -Path $outputFile -Value $Rtb.Rtf -Encoding utf8
