# PowerShell script to send UART frames via serial port
# Requires: PowerShell 5+, System.IO.Ports.SerialPort

param(
    [string]$Port = "COM2",
    [int]$Baud = 9600,
    [int]$InterByteDelayMs = 15,
    [int]$InterFrameDelayMs = 50,
    [double]$ACS712SensitivityMVA = 185.0,  # mV/A: 185=05B(5A), 100=20A, 66=30A
    [double]$VCC = 5.0,
    [double]$VoltageRMS = 220.0,            # V_rms lưới điện (VN=220, US=120)
    [int]$CurrentNoiseThreshold = 3         # peak_lsb <= threshold → coi là 0A (nhiễu)
)

function Parse-HexByte {
    param([string]$Value)
    $s = $Value.Trim()
    if ($s.StartsWith("0x", [System.StringComparison]::OrdinalIgnoreCase)) {
        $s = $s.Substring(2)
    }
    $n = [Convert]::ToInt32($s, 16)
    if ($n -lt 0 -or $n -gt 255) {
        throw "Hex byte out of range (00..FF): $Value"
    }
    return $n
}

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

function Send-Frame {
    param(
        [System.IO.Ports.SerialPort]$Ser,
        [int[]]$Frame,
        [int]$ByteDelayMs = 15,
        [int]$FrameDelayMs = 50
    )
    foreach ($b in $Frame) {
        $Ser.BaseStream.WriteByte([byte]$b)
        if ($ByteDelayMs -gt 0) {
            Start-Sleep -Milliseconds $ByteDelayMs
        }
    }
    $Ser.BaseStream.Flush()
    if ($FrameDelayMs -gt 0) {
        Start-Sleep -Milliseconds $FrameDelayMs
    }
    Write-Host ("Sent: " + (($Frame | ForEach-Object { $_.ToString('X2') }) -join ' '))
}

function Try-Read-Response {
    param(
        [System.IO.Ports.SerialPort]$Ser,
        [double]$Timeout = 1.0
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

  status
      Request current sensor readings (CMD 0x03)

  test <index>
      Test device: turn ON, wait, read sensor, turn OFF
      Example: test 1

  monitor [interval_ms]
      Continuous monitoring mode (default 500ms)
      Press any key to stop

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
$CMD_READ_SENSOR = 0x03


try {
    $ser = $null
    $opened = $false
    $ser = New-Object System.IO.Ports.SerialPort $Port, $Baud, 'None', 8, 'One'
    $ser.ReadTimeout = 200
    $ser.WriteTimeout = 200
    $ser.DtrEnable = $true
    $ser.RtsEnable = $true
    # Kiểm tra danh sách cổng khả dụng
    $availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
    Write-Host "[INFO] Các cổng serial khả dụng: $($availablePorts -join ', ')"
    if ($availablePorts -notcontains $Port) {
        Write-Host "[ERROR] Cổng ${Port} không nằm trong danh sách cổng khả dụng."
        return
    }
    try {
        $ser.Open()
        Start-Sleep -Milliseconds 300
        $ser.DiscardInBuffer()
        $ser.DiscardOutBuffer()
        $opened = $true
        Write-Host "[SUCCESS] Đã kết nối thành công tới cổng ${Port} @ ${Baud} baud."
        Print-Help
    } catch {
        Write-Host "[ERROR] Không thể kết nối tới cổng ${Port}. Chi tiết lỗi: $_"
        if ($ser -and $ser.IsOpen) { $ser.Close() }
        Write-Host "[DEBUG] Trạng thái IsOpen: $($ser.IsOpen)"
        Write-Host "[DEBUG] Exception: $($_.Exception.GetType().FullName) - $($_.Exception.Message)"
        $opened = $false
        return
    }
    $cmdBuffer = ""
    $rxBuffer = New-Object System.Collections.Generic.List[byte]
    Write-Host -NoNewline ">> "

    while ($opened) {
        if ([console]::KeyAvailable) {
            $key = [console]::ReadKey($true)
            if ($key.Key -eq 'Enter') {
                Write-Host ""
                $line = $cmdBuffer
                $cmdBuffer = ""

                if ($line) {
                    $parts = $line.Trim().Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries)
                    if ($parts.Count -gt 0) {
                        $cmd = $parts[0].ToLower()
                        if ($cmd -in @('quit','exit')) { break }
                        elseif ($cmd -in @('help','?')) { Print-Help }
                        else {
                            try {
                                $frame = $null
                                switch ($cmd) {
                                    'all' {
                                        if ($parts.Count -ne 2) { throw 'Usage: all <hex>' }
                                        $value = Parse-HexByte $parts[1]
                                        $frame = Build-Frame -Cmd $CMD_SET_ALL -Data @($value)
                                    }
                                    'single' {
                                        if ($parts.Count -ne 3) { throw 'Usage: single <index> <0|1>' }
                                        $index = [int]$parts[1]
                                        $value = [int]$parts[2]
                                        if ($index -lt 0 -or $index -gt 7) { throw 'index must be 0..7' }
                                        if ($value -notin 0,1) { throw 'value must be 0 or 1' }
                                        $frame = Build-Frame -Cmd $CMD_SET_SINGLE -Data @($index, $value)
                                    }
                                    'status' {
                                        Write-Host "[INFO] Yêu cầu đọc cảm biến..." -ForegroundColor Yellow
                                        $frame = Build-Frame -Cmd $CMD_READ_SENSOR -Data @()
                                    }
                                    'test' {
                                        if ($parts.Count -ne 2) { throw 'Usage: test <index>' }
                                        $index = [int]$parts[1]
                                        if ($index -lt 0 -or $index -gt 7) { throw 'index must be 0..7' }
                                        
                                        Write-Host "[TEST] Kiểm tra thiết bị $index..." -ForegroundColor Yellow
                                        
                                        # Turn ON
                                        $frame = Build-Frame -Cmd $CMD_SET_SINGLE -Data @($index, 1)
                                        Send-Frame -Ser $ser -Frame $frame -ByteDelayMs $InterByteDelayMs -FrameDelayMs $InterFrameDelayMs
                                        Write-Host "[TEST] Bật thiết bị $index, chờ 1s..." -ForegroundColor Yellow
                                        Start-Sleep -Milliseconds 1000
                                        
                                        # Read sensor
                                        $frame = Build-Frame -Cmd $CMD_READ_SENSOR -Data @()
                                        Send-Frame -Ser $ser -Frame $frame -ByteDelayMs $InterByteDelayMs -FrameDelayMs $InterFrameDelayMs
                                        Write-Host "[TEST] Đọc cảm biến..." -ForegroundColor Yellow
                                        Start-Sleep -Milliseconds 500
                                        
                                        # Turn OFF
                                        $frame = Build-Frame -Cmd $CMD_SET_SINGLE -Data @($index, 0)
                                        Write-Host "[TEST] Tắt thiết bị $index" -ForegroundColor Yellow
                                        $frame = $null  # Will be sent below
                                    }
                                    'monitor' {
                                        $interval = 500
                                        if ($parts.Count -eq 2) { $interval = [int]$parts[1] }
                                        Write-Host "[MONITOR] Bắt đầu theo dõi (nhấn phím bất kỳ để dừng)..." -ForegroundColor Yellow
                                        Write-Host "[MONITOR] Khoảng thời gian: ${interval}ms" -ForegroundColor Yellow
                                        
                                        while (-not [console]::KeyAvailable) {
                                            $frame = Build-Frame -Cmd $CMD_READ_SENSOR -Data @()
                                            Send-Frame -Ser $ser -Frame $frame -ByteDelayMs $InterByteDelayMs -FrameDelayMs $InterFrameDelayMs
                                            Start-Sleep -Milliseconds $interval
                                        }
                                        [console]::ReadKey($true) | Out-Null
                                        Write-Host "[MONITOR] Dừng theo dõi" -ForegroundColor Yellow
                                        $frame = $null
                                    }
                                    'raw' {
                                        if ($parts.Count -lt 2) { throw 'Usage: raw <cmd_hex> <data_hex...>' }
                                        $cmdhex = Parse-HexByte $parts[1]
                                        $data = @()
                                        if ($parts.Count -gt 2) {
                                            foreach ($x in $parts[2..($parts.Count-1)]) { $data += Parse-HexByte $x }
                                        }
                                        $frame = Build-Frame -Cmd $cmdhex -Data $data
                                    }
                                    default {
                                        throw "Unknown command. Type 'help'"
                                    }
                                }
                                if ($frame) {
                                    Send-Frame -Ser $ser -Frame $frame -ByteDelayMs $InterByteDelayMs -FrameDelayMs $InterFrameDelayMs
                                }
                            } catch {
                                Write-Host "Input error: $_"
                            }
                        }
                    }
                }
                Write-Host -NoNewline ">> "
            } elseif ($key.Key -eq 'Backspace') {
                if ($cmdBuffer.Length -gt 0) {
                    $cmdBuffer = $cmdBuffer.Substring(0, $cmdBuffer.Length - 1)
                    Write-Host -NoNewline "`b `b"
                }
            } else {
                if ($key.KeyChar -ne "`0") {
                    $cmdBuffer += $key.KeyChar
                    Write-Host -NoNewline $key.KeyChar
                }
            }
        }

        if ($ser.BytesToRead -gt 0) {
            $buf = New-Object byte[] $ser.BytesToRead
            $ser.Read($buf, 0, $buf.Length) | Out-Null
            $rxBuffer.AddRange($buf)
        }

        while ($rxBuffer.Count -gt 0) {
            $first = $rxBuffer[0]
            if ($first -eq 0xAA) {
                if ($rxBuffer.Count -ge 3) {
                    $frameCmd = $rxBuffer[1]
                    $frameLen = $rxBuffer[2]
                    $totalLen = 3 + $frameLen + 1
                    if ($frameLen -gt 15) {
                         $rxBuffer.RemoveAt(0)
                         continue
                    }
                    if ($rxBuffer.Count -ge $totalLen) {
                        $frameBytes = $rxBuffer.GetRange(0, $totalLen).ToArray()
                        $rxBuffer.RemoveRange(0, $totalLen)

                        Write-Host ""
                        if ($frameCmd -eq 0x04 -and $frameLen -eq 9) {
                            $sensitivityVA = $ACS712SensitivityMVA / 1000.0
                            $sqrt2 = [math]::Sqrt(2)
                            $channelArr = $frameBytes[3..10] | ForEach-Object {
                                $peakLsb = $_
                                if ($peakLsb -le $CurrentNoiseThreshold) {
                                    "0A"
                                } else {
                                    $I_peak = $peakLsb * $VCC / 256.0 / $sensitivityVA
                                    $I_rms  = [math]::Round($I_peak / $sqrt2, 2)
                                    $P_W    = [math]::Round($I_rms * $VoltageRMS, 1)
                                    "${I_rms}A/${P_W}W"
                                }
                            }
                            $bits = [convert]::ToString($frameBytes[11], 2).PadLeft(8, '0')
                            Write-Host "[SENSOR] $($channelArr -join ', ') | Devices: $bits" -ForegroundColor Cyan

                            for ($i = 0; $i -lt 8; $i++) {
                                $peakLsb  = $frameBytes[3 + $i]
                                $deviceOn = ($frameBytes[11] -band (1 -shl $i)) -ne 0
                                $hasCurrent = $peakLsb -gt $CurrentNoiseThreshold
                                if ($deviceOn -and -not $hasCurrent) {
                                    Write-Host "[WARNING] Thiết bị $i BẬT nhưng dòng điện = 0A!" -ForegroundColor Red
                                } elseif (-not $deviceOn -and $hasCurrent) {
                                    $I_peak = $peakLsb * $VCC / 256.0 / $sensitivityVA
                                    $I_rms  = [math]::Round($I_peak / $sqrt2, 2)
                                    Write-Host "[WARNING] Thiết bị $i TẮT nhưng vẫn có dòng = ${I_rms}A!" -ForegroundColor Red
                                }
                            }
                        } else {
                            Write-Host "[RX FRAME] Cmd: $($frameCmd.ToString('X2')) Data: $(($frameBytes[3..($totalLen-2)] | ForEach-Object { $_.ToString('X2') }) -join ' ')" -ForegroundColor Cyan
                        }
                        Write-Host -NoNewline ">> $cmdBuffer"
                    } else {
                        break
                    }
                } else {
                    break
                }
            } else {
                $idxAA = $rxBuffer.IndexOf(0xAA)
                if ($idxAA -lt 0) { $idxAA = $rxBuffer.Count }
                $textBytes = $rxBuffer.GetRange(0, $idxAA).ToArray()
                $rxBuffer.RemoveRange(0, $idxAA)

                $text = [System.Text.Encoding]::ASCII.GetString($textBytes) -replace '[^\x20-\x7E\r\n]', ''
                if ($text.Trim().Length -gt 0) {
                    Write-Host ""
                    Write-Host "[AVR TEXT] $($text.Trim())" -ForegroundColor Green
                    Write-Host -NoNewline ">> $cmdBuffer"
                }
            }
        }
        Start-Sleep -Milliseconds 10
    }
} finally {
    if ($ser -and $ser.IsOpen) { $ser.Close(); Write-Host 'Closed.' }
}