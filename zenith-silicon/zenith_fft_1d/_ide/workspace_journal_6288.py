# 2026-04-28T15:25:56.243421200
import vitis

client = vitis.create_client()
client.set_workspace(path="zenith_fft_1d")

vitis.dispose()

