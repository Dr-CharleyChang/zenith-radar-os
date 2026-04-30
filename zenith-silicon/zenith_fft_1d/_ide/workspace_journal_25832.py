# 2026-04-23T21:35:45.577353100
import vitis

client = vitis.create_client()
client.set_workspace(path="zenith_fft_1d")

vitis.dispose()

