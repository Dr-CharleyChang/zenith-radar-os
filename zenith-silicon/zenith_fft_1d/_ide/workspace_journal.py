# 2026-04-30T10:15:29.936224500
import vitis

client = vitis.create_client()
client.set_workspace(path="zenith_fft_1d")

vitis.dispose()

