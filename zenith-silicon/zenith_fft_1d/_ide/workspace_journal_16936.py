# 2026-04-23T21:38:31.972458700
import vitis

client = vitis.create_client()
client.set_workspace(path="zenith_fft_1d")

vitis.dispose()

