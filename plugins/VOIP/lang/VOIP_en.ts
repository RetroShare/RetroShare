<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video Processing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Available bandwidth:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Use this field to simulate the maximum bandwidth available so as to preview what the encoded video will look like with the corresponding compression rate.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+21"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Display encoded (and then decoded) frame, to check the codec&apos;s quality. If not selected, the image above only shows the frame that is grabbed from your camera.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>preview</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="+185"/>
        <source>Continuous</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+91"/>
        <source>%1 s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>-%1 dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="+85"/>
        <source>VOIP</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="+14"/>
        <source>Audio Statistics</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="+128"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location line="+47"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+40"/>
        <location line="+4"/>
        <location line="+4"/>
        <location line="+4"/>
        <source>VOIP</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-11"/>
        <source>Incoming audio call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Incoming video call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing audio call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing video call</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VOIPChatWidgetHolder</name>
    <message>
        <location filename="../gui/VOIPChatWidgetHolder.cpp" line="+70"/>
        <location line="+146"/>
        <source>Mute</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-128"/>
        <location line="+138"/>
        <source>Start Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Start Video Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Hangup Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-113"/>
        <location line="+626"/>
        <source>Hide Chat Text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-608"/>
        <location line="+106"/>
        <location line="+523"/>
        <source>Fullscreen mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-410"/>
        <source>Accept Audio Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Decline Audio Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse audio call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Decline Video Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse video call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+102"/>
        <source>Mute yourself</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmute yourself</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+603"/>
        <source>Your friend is calling you for video. Respond.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-467"/>
        <source>Hold Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Outgoing Call is started...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Resume Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Outgoing Audio Call stopped.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Shut camera off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>You&apos;re now sending video...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-266"/>
        <location line="+279"/>
        <source>Activate camera</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video call stopped</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-295"/>
        <source>Accept Video Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-55"/>
        <source>%1 is inviting you to start an audio conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activate audio</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+50"/>
        <source>%1 is inviting you to start a video conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+334"/>
        <source>Show Chat Text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Return to normal view.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+228"/>
        <source>%1 hang up. Your call is closed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your audio call is closed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your video call is closed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+21"/>
        <source>%1 accepted your audio call.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 accepted your video call.</source>
        <translation type="unfinished"></translation>
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
        <location line="-44"/>
        <source>Your friend is calling you for audio. Respond.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+58"/>
        <source>Answer</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VOIPPlugin</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="-48"/>
        <source>VOIP</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VOIPToasterItem</name>
    <message>
        <location filename="../gui/VOIPToasterItem.cpp" line="+42"/>
        <source>Answer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Answer with video</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/VOIPToasterItem.ui" line="+232"/>
        <source>Decline</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VOIPToasterNotify</name>
    <message>
        <location filename="../gui/VOIPToasterNotify.cpp" line="+55"/>
        <source>VOIP</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Accept</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bandwidth Information</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Audio or Video Data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>HangUp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Invitation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Audio Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Video Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Test VOIP Accept</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP BandwidthInfo</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP HangUp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Invitation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Test VOIP Audio Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Video Call</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Accept received from this peer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Bandwidth Info received from this peer: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Audio or Video Data received from this peer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+24"/>
        <source>HangUp received from this peer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Invitation received from this peer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+24"/>
        <source>calling</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>voipGraphSource</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="-246"/>
        <source>Required bandwidth</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
