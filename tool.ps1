# PowerShell script to send UART frames via serial port
# Requires: PowerShell 5+, System.IO.Ports.SerialPort

param(
    [string]$Port = "COM2",
    [int]$Baud = 9600
)

function Calc-Checksum {
    param(
        [int]$Cmd,
        [int[]]$Data
    )
    $FRAME_HEADER = 0xAA
    $length = $Data.Length
    $cs = $FRAME_HEADER -bxor $Cmd -bxor $length
    foreach ($b in $Data) { $cs = $cs -bxor $b }
    return ($cs -band 0xFF)
}

function Build-Frame {
    param(
        [int]$Cmd,
        [int[]]$Data
    )
    $FRAME_HEADER = 0xAA
    $length = $Data.Length
    $checksum = Calc-Checksum -Cmd $Cmd -Data $Data
    $arr = @($FRAME_HEADER, $Cmd, $length) + $Data + $checksum
    return [int[]]$arr
}

function Parse-HexByte {
    param(
        [string]$Text,
        [string]$Name = 'value'
    )

    $v = [Convert]::ToInt32($Text, 16)
    if ($v -lt 0 -or $v -gt 0xFF) {
        throw "$Name must be 00..FF"
    }
    return $v
}

function Send-Frame {
    param(
        [System.IO.Ports.SerialPort]$Ser,
        [int[]]$Frame
    )
    $bytes = [byte[]]$Frame
    $Ser.Write($bytes, 0, $bytes.Length)
    $Ser.BaseStream.Flush()
    Write-Host ("Sent: " + (($Frame | ForEach-Object { $_.ToString('X2') }) -join ' '))
}

function Try-Read-Response {
    param(
        [System.IO.Ports.SerialPort]$Ser,
        [double]$Timeout = 1.00
    )
    $end = (Get-Date).AddSeconds($Timeout)
    $rx = New-Object System.Collections.Generic.List[byte]
    while ((Get-Date) -lt $end) {
        if ($Ser.BytesToRead -gt 0) {
            $buf = New-Object byte[] $Ser.BytesToRead
            $Ser.Read($buf, 0, $buf.Length) | Out-Null
            $rx.AddRange($buf)
        }
        Start-Sleep -Milliseconds 10
    }
    return $rx.ToArray()
}

function Print-Help {
    Write-Host @"
Commands:
  all <hex>
      Example: all FF
      Send CMD_SET_ALL with 1 byte payload

  single <index> <0|1>
      Example: single 3 1
      index: 0..7
      value: 1=ON, 0=OFF

  raw <cmd_hex> <data_hex...>
      Example: raw 01 FF
      Example: raw 02 03 01

  quit
      Exit tool
"@
}

# Main logic
$CMD_SET_ALL = 0x01
$CMD_SET_SINGLE = 0x02


try {
    $ser = $null
    $opened = $false
    $ser = New-Object System.IO.Ports.SerialPort $Port, $Baud, 'None', 8, 1
    # Kiểm tra danh sách cổng khả dụng
    $availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
    Write-Host "[INFO] Các cổng serial khả dụng: $($availablePorts -join ', ')"
    if ($availablePorts -notcontains $Port) {
        Write-Host "[ERROR] Cổng ${Port} không nằm trong danh sách cổng khả dụng."
        return
    }
    try {
        $ser.Open()
        $ser.ReadTimeout = 100
        $ser.WriteTimeout = 100
        $ser.DiscardInBuffer()
        $ser.DiscardOutBuffer()
        $opened = $true
        Write-Host "[SUCCESS] Đã kết nối thành công tới cổng ${Port} @ ${Baud} baud."
        Print-Help

        # Thử đọc dữ liệu khởi động (ví dụ: "System Ready")
        $boot = Try-Read-Response -Ser $ser -Timeout 0.8
        if ($boot.Length -gt 0) {
            try {
                $bootText = [System.Text.Encoding]::ASCII.GetString($boot)
                Write-Host "BOOT text: $($bootText)"
            } catch {}
            Write-Host ("BOOT hex : " + (($boot | ForEach-Object { $_.ToString('X2') }) -join ' '))
        }
    } catch {
        Write-Host "[ERROR] Không thể kết nối tới cổng ${Port}. Chi tiết lỗi: $_"
        if ($ser -and $ser.IsOpen) { $ser.Close() }
        Write-Host "[DEBUG] Trạng thái IsOpen: $($ser.IsOpen)"
        Write-Host "[DEBUG] Exception: $($_.Exception.GetType().FullName) - $($_.Exception.Message)"
        $opened = $false
        return
    }
    while ($opened) {
        $line = Read-Host '>>'
        if (-not $line) { continue }
        $parts = $line.Trim().Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries)
        $cmd = $parts[0].ToLower()
        if ($cmd -in @('quit','exit')) { break }
        if ($cmd -in @('help','?')) { Print-Help; continue }
        try {
            switch ($cmd) {
                'all' {
                    if ($parts.Count -ne 2) { Write-Host 'Usage: all <hex>'; continue }
                    $value = Parse-HexByte -Text $parts[1] -Name 'all value'
                    $frame = Build-Frame -Cmd $CMD_SET_ALL -Data @($value)
                }
                'single' {
                    if ($parts.Count -ne 3) { Write-Host 'Usage: single <index> <0|1>'; continue }
                    $index = [int]$parts[1]
                    $value = [int]$parts[2]
                    if ($index -lt 0 -or $index -gt 7) { Write-Host 'index must be 0..7'; continue }
                    if ($value -notin 0,1) { Write-Host 'value must be 0 or 1'; continue }
                    $frame = Build-Frame -Cmd $CMD_SET_SINGLE -Data @($index, $value)
                }
                'raw' {
                    if ($parts.Count -lt 2) { Write-Host 'Usage: raw <cmd_hex> <data_hex...>'; continue }
                    $cmdhex = Parse-HexByte -Text $parts[1] -Name 'cmd'
                    $data = @()
                    if ($parts.Count -gt 2) {
                        foreach ($x in $parts[2..($parts.Count-1)]) {
                            $data += (Parse-HexByte -Text $x -Name 'data byte')
                        }
                    }
                    $frame = Build-Frame -Cmd $cmdhex -Data $data
                }
                default {
                    Write-Host "Unknown command. Type 'help'"; continue
                }
            }
            Send-Frame -Ser $ser -Frame $frame
            $rx = Try-Read-Response -Ser $ser
            if ($rx.Length -gt 0) {
                try {
                    $text = [System.Text.Encoding]::ASCII.GetString($rx)
                    Write-Host "RX text: $($text)"
                } catch {}
                Write-Host ("RX hex : " + (($rx | ForEach-Object { $_.ToString('X2') }) -join ' '))
            } else {
                Write-Host "RX: <no response>"
            }
        } catch {
            Write-Host "Input error: $_"
        }
    }
} finally {
    if ($ser -and $ser.IsOpen) { $ser.Close(); Write-Host 'Closed.' }
}