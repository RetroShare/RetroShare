<?xml version="1.0" ?><!DOCTYPE TS><TS language="ru" version="2.0">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="17"/>
        <source>Audio Wizard</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="30"/>
        <source>Transmission</source>
        <translation>Передача звука</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="36"/>
        <source>&amp;Transmit</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="46"/>
        <source>When to transmit your speech</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="49"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="63"/>
        <source>DoublePush Time</source>
        <translation>Задержка между нажатиями</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="73"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Если Вы нажмёте кнопку передачи речи два раза за установленное время, то включится функция залипания.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="76"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;Задержка между нажатиями&lt;/b&gt;&lt;br /&gt;Если Вы нажмёте кнопку передачи речи два раза за установленный интервал времени, включится залипание. RetroShare будет продолжать передавать звук от микрофона, пока Вы не нажмёте эту кнопку ещё раз.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="119"/>
        <source>Voice &amp;Hold</source>
        <translation>&amp;Удержание голоса</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="129"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Как долго продолжать передавать речь после наступления тишины</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="132"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Как долго после паузы в речи продолжать передачу.&lt;/b&gt;&lt;br /&gt;Увеличьте это значение, если у Вас много пауз в речи (можно увидеть по часто мигающей иконке голоса рядом с вашим именем).</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="148"/>
        <source>Silence Below</source>
        <translation>Уровень тишины</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="155"/>
        <source>Signal values below this count as silence</source>
        <translation>Сигнал ниже этого значения считается тишиной</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="158"/>
        <location filename="../gui/AudioInputConfig.ui" line="190"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Устанавливает значения для срабатывания обнаружения голоса.&lt;/b&gt;&lt;br /&gt;Используйте вместе с окном Аудиостатистики, чтобы отрегулировать эти значения вручную. Значения левее &quot;Уровень тишины&quot; всегда будут считаться тишиной. Значения правее &quot;Уровень речи&quot; всегда будет считаться голосом. Значения между будут считаться голосом если вы уже говорите, но это не будет считаться новым срабатыванием.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="180"/>
        <source>Speech Above</source>
        <translation>Уровень речи</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="187"/>
        <source>Signal values above this count as voice</source>
        <translation>Сигнал громче этого уровня будет распознан как речь</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="225"/>
        <source>empty</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="240"/>
        <source>Audio Processing</source>
        <translation>Обработка звука</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="246"/>
        <source>Noise Suppression</source>
        <translation>Подавление шума</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="259"/>
        <source>Noise suppression</source>
        <translation>Подавление шума</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="262"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Устанавливает коэффициент подавления шума.&lt;/b&gt;&lt;br /&gt;Чем выше это значение, тем более агрессивно будет подавлен шум.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="294"/>
        <source>Amplification</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="304"/>
        <source>Maximum amplification of input sound</source>
        <translation>Максимальное усиление исходящего звука</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="307"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;Максимальное усиление исходящего сигнала.&lt;/b&gt;&lt;br /&gt;RetroShare нормализует исходящую громкость до сжатия и эта опция устанавливает на сколько можно его усилить.&lt;br /&gt;Актуальный уровень постоянно обновляется на основе текущего образца речи, но никогда не будет выше установленного здесь уровня.&lt;br /&gt;Если уровень &lt;i&gt;Громкости микрофона&lt;/i&gt; аудиостатистики держится на уровне 100%, Вы можете установить его на 2.0 или выше, но если, как многие люди, Вы не можете достичь 100%, установите его на чуть более высоком уровне.&lt;br /&gt;В идеале, установите его так, чтобы &lt;i&gt;Громкость микрофона * Фактор усиления &gt;= 100&lt;/i&gt;, даже если Вы говорите слишком мягко.&lt;br /&gt;&lt;br /&gt;Заметьте, что ничего плохого не случится, если Вы установите его на максимум, но RetroShare начнет передавать другие переговоры, если Вы оставите это значение по умолчанию.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="339"/>
        <source>Echo Cancellation Processing</source>
        <translation type="unfinished"/>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="100"/>
        <source>Continuous</source>
        <translation>непрерывный</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="101"/>
        <source>Voice Activity</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="102"/>
        <source>Push To Talk</source>
        <translation>Активация по кнопке</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="204"/>
        <source>%1 s</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="212"/>
        <source>Off</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="215"/>
        <source>-%1 dB</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="72"/>
        <source>VOIP</source>
        <translation type="unfinished"/>
    </message>
</context>
<context>
    <name>AudioPopupChatDialog</name>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="23"/>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="87"/>
        <source>Mute yourself</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="43"/>
        <source>Start Call</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="65"/>
        <source>Hangup Call</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="89"/>
        <source>Unmute yourself</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="118"/>
        <source>Hold Call</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="137"/>
        <source>VoIP Status</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="137"/>
        <source>Outgoing Call is started...</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="145"/>
        <source>Resume Call</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="172"/>
        <source>Answer</source>
        <translation type="unfinished"/>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="14"/>
        <source>Audio Statistics</source>
        <translation>Аудио статистика</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="22"/>
        <source>Input Levels</source>
        <translation>Входящие уровни</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="28"/>
        <source>Peak microphone level</source>
        <translation>Пиковый уровень микрофона</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="35"/>
        <location filename="../gui/AudioStats.ui" line="55"/>
        <location filename="../gui/AudioStats.ui" line="75"/>
        <source>Peak power in last frame</source>
        <translation>Пиковая мощность в последнем фрагменте</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="38"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Показывает пиковую мощность последнего фрагмента (20 мс), и является тем же параметром, который Вы обычно видите как &quot;входящая мощность&quot;. Проигнорируйте этот параметр в пользу &lt;b&gt;Мощности микрофона&lt;/b&gt;, который является более корректным.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="48"/>
        <source>Peak speaker level</source>
        <translation>Пиковый уровень динамика</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="58"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Показывает пиковую мощность в последнем фрагменте (20 мс) динамиков. Пока Вы используете мультиканальный метод проб (такой как ASIO),настроенный на каналы динамика, она будет равна 0. Если у Вас они так сконфигурированы, все еще будет отображаться 0, пока Вы будете прослушивать звук из других программ, Ваши настройки не будут работать.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="68"/>
        <source>Peak clean level</source>
        <translation>Пиковый уровень очистки</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="78"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Показывает пиковую мощность в последнем фрейме (20 мс) после всей обработки. В идеале здесь должно быть -96 дБ, когда Вы не говорите. В реальной жизни, в студии будет около -60 дБ, но Вы увидите в лучшем случае -20 дБ. Когда Вы говорите этот показатель будет увеличиваться в районе от -5 до -10 дБ.&lt;br /&gt;Если Вы используете подавление эхо, и это значение повышается более -15 дБ, когда Вы не говорите - Ваши настройки не работают и Вы будете раздражать других пользователей эхом.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="91"/>
        <source>Signal Analysis</source>
        <translation>Анализ сигнала</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="97"/>
        <source>Microphone power</source>
        <translation>Мощность микрофона</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="104"/>
        <source>How close the current input level is to ideal</source>
        <translation>Как близок текущий входящий уровень к идеалу</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="107"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Показывает насколько близок Ваш текущий уровень входящей громкости к идеалу. Чтобы настроить громкость микрофона, откройте любую программу настройки громкости, и настройте громкость записи, затем посмотрите это значение во время разговора. &lt;br /&gt;&lt;b&gt;Говорите громко, как будто расстроены или раздражены действиями нуба.&lt;/b&gt;&lt;br /&gt;Настраивайте громкость, до тех пор, пока число не будет близко к 100%, но убедитесь, что оно не поднимется выше. Если оно будет выше, скорее всего будут прерывания Вашей речи, что ухудшит качество звука.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="117"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Соотношение Сигнал/Шум</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="124"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Соотношение Сигнал/Шум от микрофона</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="127"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>Отношение Сигнал/Шум (SNR) микрофона в последнем фрагменте (20 мс). Показывает, насколько чист голос по сравнению с шумом.&lt;br /&gt;Если значение ниже 1.0, в сигнале больше шума, нежели голоса, и поэтому ухудшается качество.&lt;br /&gt;Верхнего предела не существует, но не ожидайте увидеть выше 40-50, если Вы не в студии звукозаписи.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="137"/>
        <source>Speech Probability</source>
        <translation>Вероятность речи</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="144"/>
        <source>Probability of speech</source>
        <translation>Вероятность речи</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="147"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Вероятность того, что последний фрагмент (20 мс) был речью, а не шумом.&lt;br /&gt;Передача голосовой активности зависит от верности этого. Фишка в том, что середина предложения всегда распознается как речь; проблема в паузах между словами и началом разговора. Трудно отделить кашель от слова, начинающегося на &apos;х&apos;.&lt;br /&gt;Если это выделено жирным шрифтом, значит RetroShare сейчас передает (если Вы подключены).</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="162"/>
        <source>Configuration feedback</source>
        <translation>Текущие настройки</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="168"/>
        <source>Current audio bitrate</source>
        <translation>Текущий битрейт звука</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="181"/>
        <source>Bitrate of last frame</source>
        <translation>Битрейт последнего фрагмента</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="184"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>Аудио битрейт последнего сжатого фрагмента (20 мс), который будет меняться, как VBR (Variable Bit Rate - переменная скорость передачи). Пиковый битрейт может быть отрегулирован в Настройках.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="194"/>
        <source>DoublePush interval</source>
        <translation>Скорость двойного нажатия</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="207"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Время между последними двумя нажатиями Кнопки активации речи</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="217"/>
        <source>Speech Detection</source>
        <translation>Определение речи</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="224"/>
        <source>Current speech detection chance</source>
        <translation>Шанс определения речи</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="227"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Показывает текущие настройки определения речи.&lt;/b&gt;&lt;br /&gt;Вы можете изменить эти настройки в Настройках или в Мастере настройки звука.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="256"/>
        <source>Signal and noise power spectrum</source>
        <translation>Спектр мощности сигнала и шума</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="262"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Мощностной спектр входящего сигнала и оценка шума</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="265"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Показывает мощностной спектр текущего входящего сигнала (красная линия) и текущая оценка шума (заполнена синим).&lt;br /&gt;Все амплитуды умножены на 30, чтобы показать интересные части (на сколько больше сигнала, чем шума представлено в каждом отрезке).&lt;br /&gt;Это только если Вы интересуетесь точными условиями шума вашего микрофона. При хороших условиях, это будет всего лишь крошечным синим отрезком внизу. Если синего на графике более половины, у Вас серьезные проблемы с шумом окружающей среды.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="281"/>
        <source>Echo Analysis</source>
        <translation>Анализ Эха</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="293"/>
        <source>Weights of the echo canceller</source>
        <translation>Значение подавления эха</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="296"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Отображает значение подавления эха, со временем растущего вниз и увеличивающим частоту вправо.&lt;br /&gt;В идеале, все должно быть черным, отображая, что эха нет. В общем, у Вас будет 1 или несколько горизонтальных полосок синеватого цвета, отображающих время задержки эха. Вы должны видеть значения в реальном времени.&lt;br /&gt;Заметьте, что если у Вас нет эхо, которое нужно подавить, Вы не увидите здесь полезной информации. Запустите музыку, и все должно нормализоваться. &lt;br /&gt;Вы можете выбрать просмотр реальных или воображаемых частей доменно-частотных значений, или наоборот высчитанные модуль и фазу. Наиболее применимым из них будут, скорее всего модуль, который является амплитудой эха, и показывает, сколько сигнала было вырезано за промежуток времени. Другие режимы просмотра более полезны для людей, желающих настроить алгоритмы глушения эха.&lt;br /&gt;Пожалуйста, заметьте: Если картина в целом обширно варьируется в режиме модулей, подавление эха не может найти соответствия между двумя входными источниками (динамиками и микрофоном). Или у Вас очень долгая задержка эха, или один из входящих источников настроен неправильно.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Мастер настройки звука</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="18"/>
        <source>Introduction</source>
        <translation>Вступление</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="21"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Добро пожаловать в мастер настройки звука RetroShare</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="32"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Вас приветствует мастер настройки звука RetroShare. Он поможет Вам правильно настроить Вашу звуковую карту, а также установить наилучшие параметры для обработки звука.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="56"/>
        <source>Volume tuning</source>
        <translation>Настройка громкости</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="59"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Установите оптимальные настройки громкости микрофона.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="70"/>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there's an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it's checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation>&lt;p&gt;Откройте системную панель управления звуком и перейдите к параметрам записи. Убедитесь, что микрофон выбран в качестве активного входа с максимальной громкостью записи. Если есть возможность включить &quot;усиление Микрофона&quot; (Microphone Boost) - включите его.&lt;/p&gt;
&lt;p&gt;Говорите громко, как будто Вы раздражены или взволнованы. Уменьшите громкость звука в системной панели управления так, чтобы показания индикатора звука были максимум в синей и зеленой зоне, но &lt;b&gt;НЕ&lt;/b&gt; красной, пока Вы говорите.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="86"/>
        <source>Talk normally, and adjust the slider below so that the bar moves into green when you talk, and doesn&apos;t go into the orange zone.</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="130"/>
        <source>Stop looping echo for this wizard</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="150"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Применить высококонтрастную цветовую схему для людей со слабым зрением</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="153"/>
        <source>Use high contrast graphics</source>
        <translation>Использовать высококонтраструю цветовую схему</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="163"/>
        <source>Voice Activity Detection</source>
        <translation>Определение Голосовой активности</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="166"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>Укажите RetroShare когда Вы говорите и когда Вы молчите.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="172"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Этот шаг поможет RetroShare определить, когда Вы говорите. Сначала выберите - какие данные для этого использовать.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="184"/>
        <source>Push To Talk:</source>
        <translation>Активация по кнопке:</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="213"/>
        <source>Voice Detection</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="226"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>Затем Вы должны откорректировать бегунок ниже. С начала произнесения речи индикатор НЕ должен выходить на зеленую зону (Ваша речь). Когда Вы продолжаете говорить, индикатор должен остаться в желтой зоне (могла бы быть Ваша речь) и когда Вы молчите, индикатор должен оставаться в красной зоне (не речь).</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="290"/>
        <source>Continuous transmission</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="298"/>
        <source>Finished</source>
        <translation>Закончить</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="301"/>
        <source>Enjoy using RetroShare</source>
        <translation>Наслаждайтесь использованием RetroShare</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="312"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Поздравляем. Теперь Вы готовы насладиться более богатым звуком с RetroShare.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="94"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="95"/>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="96"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="97"/>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="98"/>
        <source>&lt;li&gt; in the private chat, enable sound input/output by clicking on the two VOIP icons&lt;/li&gt;&lt;/ul&gt;</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="99"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="100"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="118"/>
        <source>RTT Statistics</source>
        <translation>RTT статистика</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="145"/>
        <location filename="../gui/VoipStatistics.cpp" line="147"/>
        <location filename="../gui/VoipStatistics.cpp" line="149"/>
        <source>secs</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="151"/>
        <source>Old</source>
        <translation>старый</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="152"/>
        <source>Now</source>
        <translation>сейчас</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="361"/>
        <source>Round Trip Time:</source>
        <translation type="unfinished"/>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="154"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation type="unfinished"/>
    </message>
</context>
<context>
    <name>VOIPPlugin</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="159"/>
        <source>VOIP</source>
        <translation type="unfinished"/>
    </message>
</context>
<context>
    <name>VoipStatistics</name>
    <message>
        <location filename="../gui/VoipStatistics.ui" line="14"/>
        <source>VoipTest Statistics</source>
        <translation>Voip тест Статистика</translation>
    </message>
</context>
</TS>