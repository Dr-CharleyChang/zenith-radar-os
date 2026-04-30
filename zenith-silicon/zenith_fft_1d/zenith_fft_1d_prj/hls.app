<AutoPilot:project xmlns:AutoPilot="com.autoesl.autopilot.project" projectType="C/C++" name="zenith_fft_1d_prj" ideType="classic" top="fft_1d_top">
    <Simulation argv="">
        <SimFlow name="csim" setup="false" optimizeCompile="false" clean="false" ldflags="" mflags=""/>
    </Simulation>
    <files>
        <file name="../../tb/fft_1d_tb.cpp" sc="0" tb="1" cflags="-Wno-unknown-pragmas" csimflags="" blackbox="false"/>
        <file name="src/fft_1d_hls.cpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src/fft_1d.hpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src/fft_1d.cpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
    </files>
    <solutions>
        <solution name="solution1" status=""/>
    </solutions>
</AutoPilot:project>
