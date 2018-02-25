<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja_JP">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation>オーディオ設定ウィザード</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation>伝送方式</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation>送信(&amp;T)</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation>発言を送信するタイミング</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;発言を送信するタイミングを設定します。&lt;/b&gt;&lt;br /&gt;&lt;i&gt;常に有効&lt;/i&gt; - 常時音声入力を受け付けます&lt;br /&gt;&lt;i&gt;声で有効化&lt;/i&gt; - はっきりとしゃべっている時に受け付けます。&lt;br /&gt;&lt;i&gt;プッシュ・トゥ・トーク&lt;/i&gt; - &lt;i&gt;ショートカットキー&lt;/i&gt;で設定したキーを押している間音声入力を受け付けます。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation>二重押しの時間</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>プッシュ・トゥ・トーク キーを2回押すとロックされます。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;2度押し時間&lt;/b&gt;&lt;br /&gt;もしあなたがプッシュ・トゥ・トークキーを2回、ここで設定した感覚の間に押したなら、キーはロックされます。RetroShare は あなたがプッシュ・トゥ・トークをアンロックするために一回以上キーを押すまで転送状態のままです。</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation>送信継続時間(&amp;H)</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation>音声が無くなった後、どれくらい送信を続けるか</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;発言の転送をどれだけ継続するべきかを設定します。&lt;/b&gt;&lt;br /&gt;あなたの声が発言中に破綻する（あなたの名前の次にある音声アイコンがすぐに点滅する場合）ならにこの値を高く設置してください。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation>非発言のしきい値</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation>発言していないと判定される信号値のしきい値</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;声を検出するための基準値を設定します。&lt;/b&gt;手動で調整するためには音声統計ウインドウを一緒にご利用ください。&quot;非発言しきい値&quot;以下の値は常に発言していない状態と見なされ、&quot;発言しきい値&quot;より上の値は発言と見なされます。これらの間の値は既に話し中であれば発言と判断されますが、新たな発言であると判断する材料にはなりません。</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation>発言しきい値</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation>発言と判定される信号値のしきい値</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation>空</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation>音声処理</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation>ノイズ抑制</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation>ノイズ抑制</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;この設定は多くのノイズを抑制します。&lt;/b&gt;&lt;br /&gt;より大きい値を設定すると、より多くのノイズが抑制されます。</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation>増幅</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation>音声入力の最大増幅量</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;音声入力の最大増幅量です。&lt;/b&gt;&lt;br /&gt;RetroShareは圧縮を行う前に入力量を正常化します。この設定は、どれくらい音量を増幅させるかを決定します。&lt;br /&gt;実際の音量はその時の発言パターンによって変化しますが、ここで指定された音量を上回ることはありません。&lt;br /&gt;音声統計で&lt;i&gt;マイクの音量&lt;/i&gt;レベルが常に100%近くに達しているなら、この値を2.0くらいにしたくなるかも知れません。しかし、(大部分の人がそうだと思いますが)100%に届かないなら、この値をより高くしてください。&lt;br /&gt;あなたが非常に穏やかに話している時に&lt;i&gt;マイク音量 * 増幅値 &gt;= 100&lt;/i&gt; となるのが理想的です。&lt;br /&gt;&lt;br /&gt;この値を最大にすることに実害はありませんが、だからといって適当に最大値に合わせてしまうと他の会話まで拾ってしまうので気をつけてください。</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation>エコーキャンセル処理</translation>
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
        <location filename="../gui/AudioInputConfig.cpp" line="+202"/>
        <source>Continuous</source>
        <translation>続行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation>発声</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation>プッシュ・トゥ・トーク</translation>
    </message>
    <message>
        <location line="+105"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation>オフ</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>-%1 dB</source>
        <translation>-%1 dB</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="+94"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="+14"/>
        <source>Audio Statistics</source>
        <translation>音声統計</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation>入力レベル</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation>最大マイクレベル</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation>直近フレームでの最大の強さ</translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>直近のフレーム(20 ms)での最大の強さを表示します。そしてこれは、通常&quot;入力の強さ&quot;として表示されている測定値と同じものです。これは無視して&lt;b&gt;マイクの強さ&lt;/b&gt;を見るようにしてください。こちらの方がより安定していて外部要素に影響されにくいです。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation>最大スピーカーレベル</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>直近のフレーム(20 ms)におけるスピーカーの最大音量を表示します。スピーカの設定を変更して多重チャンネルサンプリング法(例えばASIO)を使用していない限り、これは0になります。セットアップでそのような構成をしているのに、ほかのプログラムが音声を再生している間も0を表示しているなら、セットアップは正しく動いていません。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation>最大クリーンレベル</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>直近のフレーム(20 ms)における全ての処理後の最大音量を表示します。あなたが話していないとき、これは -96 dBであるのが理想です。しかし実際には音楽スタジオでも -60 dB、あなたの環境では -20 dB くらい出れば良い方でしょう。あなたが話している時は、-5 から -10 dB のあたりまで上がります。&lt;br /&gt;もしあなたがエコーキャンセルを使っていて、話してもいないのに -15 dB まで上がるようならセットアップは失敗しています。この場合、反響によってほかのプレイヤーを悩ますことになります。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation>信号分析</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation>マイクの強さ</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation>入力レベルの理想値までの差</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>現在の入力音量が理想の値からどれくらい離れているかを表します。マイクの音量を調整するためには、録音ボリュームを調整するプログラムを開いて、話している間この値を見てください。&lt;br /&gt;&lt;b&gt;noob(訳注: 初心者の事)にやられて怒り狂っているときのように大声で叫んでください。&lt;/b&gt;&lt;br /&gt;この値が100%近くになるまで音量を調整してください。しかし100%を超えないように注意しましょう。もし超えてしまうとあなたの音声の一部が省略されて、音質が低下するでしょう。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation>S/N比</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>マイクからのS/N比</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>直近のフレーム(20 ms)におけるマイクの信号対雑音比(S/N比)です。これは、声がノイズと比較してどれくらいはっきりしているかを表しています。&lt;br /&gt;もしこの値が1.0以下なら、信号の中に声よりもノイズのほうが多くなっており、音質は低下しているでしょう。&lt;br /&gt;この値に上限はありませんが、音楽スタジオでもなければ40-50を大きく上回るとは思わないでください。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation>発言確率</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation>発言確率</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>直近のフレーム(20 ms)が発言であったり、環境ノイズ以外の音であった確率です。&lt;br /&gt;声で有効化する送信方式はこの値が正しい事に依存します。この事による優れた点は話の途中は常に発言と見なされることです。問題は、単語と発言開始の間にある休止です。ため息と&apos;h&apos;で始まる単語を見分けるのは難しい事です。&lt;br /&gt;これが太字になっていたら、RetroShareが現在送信中である事を表しています(接続中の場合)。</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation>設定フィードバック</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation>現在の音声ビットレート</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation>直近フレームのビットレート</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>直近の圧縮されたフレーム(20 ms)における音声ビットレートです。VBRが品質を調整するので値は上下します。ピークビットレートを調整するには、設定ダイアログで調整できます。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation>二重押しの間隔</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>直近2回のプッシュ・トゥ・トークの間隔</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation>発言検出</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation>現在の発言検出見込み</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;現在の発言検出設定を表します。&lt;/b&gt;&lt;br /&gt;設定ダイアログか音声ウィザードから設定を変更することができます。</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation>信号とノイズのパワースペクトル</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>入力信号とノイズ予想のパワースペクトル</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>現在の入力信号(赤線)とノイズ予想(青線)のパワースペクトルを表します。&lt;br /&gt;すべての振幅は判りやすい結果(各周波帯でノイズよりどれくらい多くの信号が存在するか)を提供するために30倍されます。&lt;br /&gt;マイクの雑音状況を改善しようと微調整しているならば、多分に興味深いものとなるでしょう。良好な状態では、一番下に小さく揺れている青線だけがあるべきです。もし青線がグラフの中ほどより上にあるなら、あなたは深刻にノイズの多い環境にあると言えます。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation>反響分析</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation>エコーキャンセラの重み</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>下に向かっている時間と右に向かっている周波数でエコーキャンセラの重みを表示します。&lt;br /&gt;理想的には、これは反響が全くない事を示す黒でなくてはなりません。普通は、反響が遅れる時間を表す青っぽい色の横縞が一つ以上あるでしょう。あなたは、重みがリアルタイムに更新されるのを見る事ができます。&lt;br /&gt;反響しないものがない限り、ここでは何も有用なデータが得られない事に注意してください。安定した状態で何曲かの音楽を再生してください。&lt;br /&gt;あなたは周波数領域における重みの実数または虚数の部分、もしくは計算された係数と位相の表示を選んで見ることができます。これらで最も役に立つのはおそらく係数です。係数とは反響の大きさであり、送信方向の信号が一定時間毎にどのくらい取り除かれているかを表します。他の表示は主にエコーキャンセラアルゴリズムを調整したい人にとって役に立ちます。&lt;br /&gt;注意： 係数モードの時に全ての表示が大幅に変動する場合、エコーキャンセラは2つの入力源(スピーカーとマイク)の間にどんな相関関係も見出せていません。反響に非常に長い遅れがあるか、入力源のうち1つが誤って設定されているかのどちらかでしょう。</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation>音声調整ウィザード</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation>イントロダクション</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>RetroShareオーディオウィザードにようこそ</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>これはRetroShareのオーディオ設定ウィザードです。これは、サウンドカードの入力レベルを適切にしたり、RetroShareの音声処理を正しく設定する助けになります。</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation>音量調整</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>マイクの音量を最適な設定になるよう調整します。</translation>
    </message>
    <message>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="vanished">&lt;p &gt;サウンドコントロールパネルを開き、録音設定に進んでください。録音ボリュームが最大か、マイクが選択されているかを確認して下さい。マイクブーストのオプションがある場合は、有効にしてください。&lt;/p&gt;
&lt;p&gt;イライラしたり、興奮している時のような大声でも、レッドゾーンに届かないよう、音量を下げてください。&lt;/p&gt;</translation>
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
        <translation>普通に話した時に、バーが緑へ移動し、オレンジのゾーンには入らないよう、スライダーを調整してください。</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation>このウィザードのためにルーピングエコーを停止</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>視覚障害者のためにハイコントラストへ最適化します</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation>ハイコントラストの画像を使う</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation>発声の検出</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>話しているときと、話していないときを検出する</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation>プッシュ・トゥ・トーク</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation>音声の検出</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>次に、以下の2つのスライダーを調節しましょう。最初の発声は緑(確実に発言と判定)になると良いでしょう。話している最中は黄色(発言だろうと判定)の中に収まり、話していない時はすべて赤(発言ではないと判定)に入っているようにしてください。</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation>完了</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation>RetroShareを楽しんでください</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>おめでとうございます。あなたは今、Retroshareで豊かなサウンド体験を楽しむ準備ができました。</translation>
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
        <translation type="unfinished">VOIP</translation>
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
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>VOIPToasterItem</name>
    <message>
        <location filename="../gui/VOIPToasterItem.cpp" line="+43"/>
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
        <location filename="../gui/VOIPToasterNotify.cpp" line="+56"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
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
        <location filename="../gui/AudioInputConfig.cpp" line="-260"/>
        <source>Required bandwidth</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
