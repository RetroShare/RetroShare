<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation>语音向导</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation>送话</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation>送话(&amp;T)</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation>何时传输语音</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;此项设置何时发送语音。&lt;/b&gt;&lt;br /&gt;&lt;i&gt;持续传输&lt;/i&gt; - 持续传输音频&lt;br /&gt;&lt;i&gt;语音激活&lt;/i&gt; - 检测到语音信号时传输。&lt;br /&gt;&lt;i&gt;按键送话&lt;/i&gt; - 按住&lt;i&gt;快捷键&lt;/i&gt;中指定的送话热键时传输 。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation>双击间隔</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>在此周期内连续按动送话键两次，会锁定送话状态。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;双击间隔&lt;/b&gt;&lt;br /&gt;如果您在设置的时间周期内连续按动送话键两次，会锁定送话状态，直到您再次连续按动送话键解除锁定。</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation>语音等待(&amp;H)</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation>语音停止多久后停止传输</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;此项设置程序检测到语音消失后多久停止传输音频。&lt;/b&gt;&lt;br /&gt;如果您说话时语音断续，此项可设长一些(表现为您昵称旁的语音图标频繁闪烁)。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation>静音门限</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation>此信号强度以下的声音作为静音</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;此项设置语音检测的激活强度。&lt;/b&gt;&lt;br /&gt;此项配合 声音统计 窗口使用可以手动调节语音检测的触发。低于 &quot;静噪门限&quot; 的声音总是被作为静音处理。强度高于&quot;语音门限&quot; 的声音总是被当作语音处理。两门限之间的声音在您讲话时会当作语音传输，但不会触发新的语音检测。</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation>语音门限</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation>此信号强度以上的声音作为语音</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation>空</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation>声音处理</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation>噪音抑制</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation>噪音抑制</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;此项设置应用的噪音抑制强度。&lt;/b&gt;&lt;br /&gt;数值越大，静态噪音受到的抑制约大。</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation>放大</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation>输入音频的最大放大值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;输入音频最大放大值。&lt;/b&gt;&lt;br /&gt;Mumble 在压缩声音前对音频进行正常化，此项设置允许的最大放大量。&lt;br /&gt;实际的放大级别根据您当前的语音模式持续调整，但总是在这里指定的最大值之内。&lt;br /&gt;如果“音频统计”中 &lt;i&gt;话筒响度&lt;/i&gt; 级别总是在 100% 左右，建议您将此项设置为 2.0 左右。但如果您跟多数人一样无法达到 100%，就将此项调大。&lt;br /&gt;理想情况下将此设置为 &lt;i&gt;麦克风响度 * 放大因数 &gt;= 100&lt;/i&gt;，即使您此时在轻声讲话。&lt;br /&gt;&lt;br /&gt;注意将此项设置为最大并无大碍。但如果对程序的自动调整音量不加限制，RetroShare 会将放大后的对应强度的声音作为语音。</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation>回声抑制</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video Processing</source>
        <translation>声音处理</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Available bandwidth:</source>
        <translation>可用带宽</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Use this field to simulate the maximum bandwidth available so as to preview what the encoded video will look like with the corresponding compression rate.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;使用这个部件模拟最大可用带宽来让加密视频更流畅播放&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB/s</source>
        <translation>KB/s</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Display encoded (and then decoded) frame, to check the codec&apos;s quality. If not selected, the image above only shows the frame that is grabbed from your camera.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;展示</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>preview</source>
        <translation>预览</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="+202"/>
        <source>Continuous</source>
        <translation>连续</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation>语音检测</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation>按键送话</translation>
    </message>
    <message>
        <location line="+105"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>-%1 dB</source>
        <translation>-%1 dB</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="+94"/>
        <source>VOIP</source>
        <translation>语音</translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="+14"/>
        <source>Audio Statistics</source>
        <translation>语音状态统计</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation>输入声音强度</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation>麦克风峰值强度</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation>上一帧中峰值强度</translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>此项显示上一帧(20ms)中的最大响度，与您平时看到的输入强度是一个意思，请忽略此项，&lt;b&gt;麦克风强度&lt;/b&gt;更有参考意义。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation>扬声器峰值强度</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>此项显示上一帧中扬声器的最大响度(20ms)。除非您配置了扬声器通道，正在使用多通道采样(例如ASIO)，此项将显示0。如果您采用了这种设置，而此项在您从其他程序中播放音频时仍显示0，则您的设置没有奏效。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation>处理后峰值强度</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>此项显示经过全部处理后上一帧(20ms)中的峰值响度。当您未说话时，理想值为 -96dB。实际使用中，音频工作室中可以达到 -60dB，您看到的数值大概在 -20dB 左右。当您说话时，此值将在 -5~-10dB之间。&lt;br /&gt;如果您正在使用回声抑制，在您未说话时，此项超过 -15dB ，则您的语音设置有问题，对方可能听到严重的回声。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation>信号分析</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation>麦克风强度</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation>当前输入声音强度与理想强度的差距</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>本项显示您当前的输入音量与理想音量的差距。调节音量麦克风时，请打开麦克风音量控制程序，并在讲话时观察这里的音量显示。&lt;br /&gt;&lt;b&gt;调节音量，直到您大声讲话时的音量在100%以内，否则可能降低语音质量。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation>信噪比</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>麦克风的信噪比</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>上一帧(20ms)中麦克风的信噪比(SNR)。它显示了与背景噪音相比，语音的清晰度。&lt;br /&gt;如果此值小于 1.0，噪音大于语音，因此音质不佳。&lt;br /&gt;此值没有上限，不过除非在录音棚里它很难超过 40-50。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation>语音可能性</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation>声音信号是语音的可能性</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>上一帧(20ms)中音频为语音信号而非背景噪音的可能性。&lt;br /&gt;语音活动传输依靠此功能。其中的奥妙在于连续的声音总是被检测为语音。问题是语句间的停顿和说话的起始。很难将叹气声与 &apos;h&apos; 音区分开。&lt;br /&gt;此值为加粗字体时表示 RetroShare 正在传输语音。(假设您已经建立连接)。</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation>配置反馈</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation>当前语音速率</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation>上一帧速率</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>上一帧(20ms)中音频的码率，由于采用VBR编码，码率会随音质变动。最高码率可在设置对话框中调整。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation>双击间隔</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>上次双击送话键的间隔</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation>语音检测</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation>当前检测到的语音概率</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;这显示了当前的语音检测设置。&lt;/b&gt;&lt;br /&gt;您可以从设置对话框或语音向导中更改设置。</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation>信噪强度波谱</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>输入信号与估算噪音的强度波谱</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>这显示了当前输入信号(红色线)和电流噪声估值(填充蓝色)的功率谱。&lt;br /&gt;所有的幅度乘以30，就是可能有用的部分 (显示的是每个波段信号和噪音的比例)。&lt;br /&gt;如果你想调整麦克风的噪音，那这就可用到这个。在良好的条件，底部应该只是一个蓝色的微小颤动。如果蓝色面积超过一半，那么你的环境噪音就太大了。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation>回声分析</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation>回声抑制权重</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>这个能显示回声消除器的比重，时间向下增加，频率向右增加。&lt;br /&gt;理想情况下这应该是黑色的，表示无回声的存在。一般常见的是，有一个或多个偏蓝色的横条纹，代表时间延迟回声。你应该能看到这比例是实时更新的。&lt;br/&gt;请注意，只要没有任何回音，在这里你是不会看到有用的数据。播放一些音乐或干别的什么应该会稳定。 &lt;br /&gt;你可以选择查看频域比重的实或虚部分，或交替计算出的弹性模量和相位。其中最有用的可能会是弹性模量，这代表回波的振幅，并显示有多少输出信号在该时间步长会被删除。其他能看到的东西大多数只对想调整回声消除算法的人有用。&lt;br/&gt;请注意：如果在弹性模量模式中整个图像有大规模波动，那么代表回声消除未能将两个输入源(扬声器和麦克风)关联起来。要么是回声延迟太大，要么是某一输入源配置错误。</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation>语音调节向导</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation>介绍</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>欢迎使用 RetroShare 音频向导</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>这里是 RetroShare 的音频调节向导。它将帮助您正确设置声卡的音量输入。校正 Retroshare 的声音处理参数。</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation>音量调节</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>调整麦克风硬件音量至最佳设置。</translation>
    </message>
    <message>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="vanished">&lt;p &gt;打开声音控制面板，录音设置。确定麦克风已设置为当前输入设备，且音量为最大。如果有 &amp;quot;麦克风增强选项（Microphone boost）&amp;quot; 请选中。&lt;/p&gt;
&lt;p&gt;大声说话，就像您谈到兴奋处时那样，降低声音控制面板中的音量，将下面的指示条控制在绿色和橙色区域的最大值区域，但没有进入红色区域。&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&lt;p&gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Talk normally, and adjust the slider below so that the bar moves into green when you talk, and doesn&apos;t go into the orange zone.</source>
        <translation>调节下面的滑竿令您正常说话时指示条处于绿色区间但未进入橙色区。</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation>停止向导中的循环回放</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>高对比度显示方便视觉障碍用户。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation>使用高对比度图像</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation>语音活动检测</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>告诉 RetroShare 如何判断您的通话状态。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>帮助 Retroshare 判断您何时处于说话状态。首先请选择判断方式。</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation>按键送话:</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation>语音检测</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>下面您需要调整以下滑竿。您每刚开始说话时指示条应处于绿色区域(代表激活语音状态)，您说话中间，指示条应处于黄色区域内(代表维持语音状态)，您停止说话时指示条应处于红色区域内(代表中止语音状态)</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation>连续传输</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation>完成</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation>RetroShare 使用愉快</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>恭喜您，现在您可以在Retroshare中使用语音了（私聊窗口中同时按下麦克风和耳麦按钮）。</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="+128"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;RetroShare VOIP 插件&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="vanished">&lt;br/&gt; VOIP 插件为 RetroShare 私聊窗口提供 VOIP 功能，请按如下方法使用：&lt;UL&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt; 通过配置面板，设置麦克风音量等级&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation type="vanished">&lt;li&gt; 您可以通过观察音量指示可检查麦克风是否工作正常。&lt;/li&gt;</translation>
    </message>
    <message>
        <location line="-1"/>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. To use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; check your microphone by looking at the VU-meters&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;li&gt; in the private chat, enable sound input/output by clicking on the two VOIP icons&lt;/li&gt;&lt;/ul&gt;</source>
        <translation>&lt;li&gt; 私聊中，点击这两个口形和耳机状的 VOIP 图标即可启用音频的输入输出。&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>当然，您的好友需要启用此插件才能与您接收和发送语音。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;这是一项实验性功能。您有任何意见建议请发送给 RS 开发小组。</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location line="+47"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>本插件为 RetroShare 好友间提供语音通信支持。</translation>
    </message>
    <message>
        <location line="+40"/>
        <location line="+4"/>
        <location line="+4"/>
        <location line="+4"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
    <message>
        <location line="-11"/>
        <source>Incoming audio call</source>
        <translation>VOIP语音呼入</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Incoming video call</source>
        <translation>VOIP视频呼入</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing audio call</source>
        <translation>VOIP语音呼出</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing video call</source>
        <translation>VOIP视频呼出</translation>
    </message>
</context>
<context>
    <name>VOIPChatWidgetHolder</name>
    <message>
        <location filename="../gui/VOIPChatWidgetHolder.cpp" line="+70"/>
        <location line="+146"/>
        <source>Mute</source>
        <translation>静音</translation>
    </message>
    <message>
        <location line="-128"/>
        <location line="+138"/>
        <source>Start Call</source>
        <translation>拨打</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Start Video Call</source>
        <translation>开始视频通话</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Hangup Call</source>
        <translation>挂断</translation>
    </message>
    <message>
        <location line="-113"/>
        <location line="+626"/>
        <source>Hide Chat Text</source>
        <translation>隐藏聊天记录</translation>
    </message>
    <message>
        <location line="-608"/>
        <location line="+106"/>
        <location line="+523"/>
        <source>Fullscreen mode</source>
        <translation>全屏模式</translation>
    </message>
    <message>
        <source>%1 inviting you to start an audio conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 邀请您加入语音聊天，您接受还是拒绝邀请？</translation>
    </message>
    <message>
        <location line="-410"/>
        <source>Accept Audio Call</source>
        <translation>接受语音聊天</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Decline Audio Call</source>
        <translation>拒绝语音聊天</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse audio call</source>
        <translation>拒绝语音聊天</translation>
    </message>
    <message>
        <source>%1 inviting you to start a video conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 邀请您加入视频聊天，您接受还是拒绝邀请？</translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Decline Video Call</source>
        <translation>拒绝视频聊天</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse video call</source>
        <translation>拒绝视频聊天</translation>
    </message>
    <message>
        <location line="+102"/>
        <source>Mute yourself</source>
        <translation>关闭麦克风</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmute yourself</source>
        <translation>开启麦克风</translation>
    </message>
    <message>
        <source>Waiting your friend respond your video call.</source>
        <translation type="vanished">等待对方回应您的请求</translation>
    </message>
    <message>
        <location line="+603"/>
        <source>Your friend is calling you for video. Respond.</source>
        <translation>您的朋友正在呼叫您</translation>
    </message>
    <message>
        <location line="-781"/>
        <location line="+53"/>
        <location line="+188"/>
        <location line="+24"/>
        <location line="+57"/>
        <location line="+28"/>
        <location line="+284"/>
        <location line="+11"/>
        <location line="+11"/>
        <location line="+21"/>
        <location line="+11"/>
        <source>VoIP Status</source>
        <translation>VoIP 状态</translation>
    </message>
    <message>
        <location line="-467"/>
        <source>Hold Call</source>
        <translation>呼叫保持</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Outgoing Call is started...</source>
        <translation>呼叫发出已开始</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Resume Call</source>
        <translation>回拨</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Outgoing Audio Call stopped.</source>
        <translation>呼叫发出已停止</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Shut camera off</source>
        <translation>关闭摄像头</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>You&apos;re now sending video...</source>
        <translation>您正在发送视频</translation>
    </message>
    <message>
        <location line="-266"/>
        <location line="+279"/>
        <source>Activate camera</source>
        <translation>启用摄像头</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video call stopped</source>
        <translation>视频呼叫已停止</translation>
    </message>
    <message>
        <location line="-295"/>
        <source>Accept Video Call</source>
        <translation>接受语音呼叫</translation>
    </message>
    <message>
        <location line="-55"/>
        <source>%1 is inviting you to start an audio conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activate audio</source>
        <translation>启用麦克风</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>%1 is inviting you to start a video conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+334"/>
        <source>Show Chat Text</source>
        <translation>显示聊天记录</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Return to normal view.</source>
        <translation>恢复至正常大小</translation>
    </message>
    <message>
        <location line="+228"/>
        <source>%1 hang up. Your call is closed.</source>
        <translation>%1 已挂断</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your audio call is closed.</source>
        <translation>%1 已挂断</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your video call is closed.</source>
        <translation>%1 已挂断</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>%1 accepted your audio call.</source>
        <translation>%1 已接受邀请</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 accepted your video call.</source>
        <translation>%1 已接受邀请</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Waiting for your friend to respond to your audio call.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+58"/>
        <source>Waiting for your friend to respond to your video call.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Waiting your friend respond your audio call.</source>
        <translation type="vanished">等待对方回应您的请求</translation>
    </message>
    <message>
        <location line="-44"/>
        <source>Your friend is calling you for audio. Respond.</source>
        <translation>您的朋友正在呼叫您</translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+58"/>
        <source>Answer</source>
        <translation>接受</translation>
    </message>
</context>
<context>
    <name>VOIPPlugin</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="-48"/>
        <source>VOIP</source>
        <translation>语音</translation>
    </message>
</context>
<context>
    <name>VOIPToasterItem</name>
    <message>
        <location filename="../gui/VOIPToasterItem.cpp" line="+43"/>
        <source>Answer</source>
        <translation>接受</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Answer with video</source>
        <translation>接受视频聊天</translation>
    </message>
    <message>
        <location filename="../gui/VOIPToasterItem.ui" line="+232"/>
        <source>Decline</source>
        <translation>拒绝</translation>
    </message>
</context>
<context>
    <name>VOIPToasterNotify</name>
    <message>
        <location filename="../gui/VOIPToasterNotify.cpp" line="+56"/>
        <source>VOIP</source>
        <translation>语音</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Accept</source>
        <translation>接受</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bandwidth Information</source>
        <translation>带宽信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Audio or Video Data</source>
        <translation>语音视频文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>HangUp</source>
        <translation>挂断</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Invitation</source>
        <translation>邀请</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Audio Call</source>
        <translation>语音聊天</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Video Call</source>
        <translation>视频聊天</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Test VOIP Accept</source>
        <translation>测试VOIP接受</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP BandwidthInfo</source>
        <translation>测试VOIP带宽</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Data</source>
        <translation>测试VOIP数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP HangUp</source>
        <translation>测试VOIP挂断</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Invitation</source>
        <translation>测试VOIP邀请</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Test VOIP Audio Call</source>
        <translation>测试VOIP语音聊天</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Video Call</source>
        <translation>测试VOIP视频聊天</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Accept received from this peer.</source>
        <translation>对方已接收</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Bandwidth Info received from this peer: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bandwidth Info received from this peer:%1</source>
        <translation type="vanished">已获取对方:%1带宽信息</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Audio or Video Data received from this peer.</source>
        <translation>接受对方语音视频文件</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>HangUp received from this peer.</source>
        <translation>挂断对方聊天</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Invitation received from this peer.</source>
        <translation>接受对方邀请</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+24"/>
        <source>calling</source>
        <translation>呼叫</translation>
    </message>
</context>
<context>
    <name>voipGraphSource</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="-260"/>
        <source>Required bandwidth</source>
        <translation>需要带宽</translation>
    </message>
</context>
</TS>
