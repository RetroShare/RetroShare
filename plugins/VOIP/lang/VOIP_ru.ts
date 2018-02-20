<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation>Мастер настройки звука</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation>Передача звука</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation>&amp;Передавать</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation>Время передачи речи</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;Устанавливает интервалы времени, когда речевой сигнал подлежит передаче.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Непрерывно&lt;/i&gt; — в течение всего времени&lt;br /&gt;&lt;i&gt;Речевая активность&lt;/i&gt; — когда вы говорите отчётливо и громко.&lt;br /&gt;&lt;i&gt;Нажать для разговора&lt;/i&gt; — когда вы удерживаете горячую клавишу из &lt;i&gt;Сочетание клавиш&lt;/i&gt;.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation>Задержка между нажатиями</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Если Вы нажмёте кнопку передачи речи два раза за установленное время, то включится функция залипания.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;Задержка между нажатиями&lt;/b&gt;&lt;br /&gt;Если Вы нажмёте кнопку передачи речи два раза за установленный интервал времени, включится залипание. RetroShare будет продолжать передавать звук от микрофона, пока Вы не нажмёте эту кнопку ещё раз.</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation>&amp;Удержание голоса</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Как долго продолжать передачу речи после наступления тишины</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Как долго после паузы в речи продолжать передачу.&lt;/b&gt;&lt;br /&gt;Увеличьте это значение, если у Вас много пауз в речи (можно увидеть по часто мигающей иконке голоса рядом с Вашим именем).</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation>Уровень тишины</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation>Сигнал ниже этого значения считается тишиной</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Устанавливает значения для срабатывания обнаружения голоса.&lt;/b&gt;&lt;br /&gt;Используйте вместе с окном Аудиостатистики, чтобы отрегулировать эти значения вручную. Значения левее &quot;Уровень тишины&quot; всегда будут считаться тишиной. Значения правее &quot;Уровень речи&quot; всегда будет считаться голосом. Значения между будут считаться голосом если вы уже говорите, но это не будет считаться новым срабатыванием.</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation>Уровень речи</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation>Сигнал громче этого уровня будет распознан как речь</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation>пустой</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation>Обработка звука</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation>Подавление шума</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation>Подавление шума</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Устанавливает коэффициент подавления шума.&lt;/b&gt;&lt;br /&gt;Чем выше это значение, тем более агрессивно будет подавлен шум.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation>Максимальное усиление исходящего звука</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;Максимальное усиление исходящего сигнала.&lt;/b&gt;&lt;br /&gt;RetroShare нормализует исходящую громкость до сжатия и эта опция устанавливает на сколько можно его усилить.&lt;br /&gt;Актуальный уровень постоянно обновляется на основе текущего образца речи, но никогда не будет выше установленного здесь уровня.&lt;br /&gt;Если уровень &lt;i&gt;Громкости микрофона&lt;/i&gt; аудиостатистики держится на уровне 100%, Вы можете установить его на 2.0 или выше, но если, как многие люди, Вы не можете достичь 100%, установите его на чуть более высоком уровне.&lt;br /&gt;В идеале, установите его так, чтобы &lt;i&gt;Громкость микрофона * Фактор усиления &gt;= 100&lt;/i&gt;, даже если Вы говорите слишком мягко.&lt;br /&gt;&lt;br /&gt;Заметьте, что ничего плохого не случится, если Вы установите его на максимум, но RetroShare начнет передавать другие переговоры, если Вы оставите это значение по умолчанию.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation>Обработка Отмены Эхо</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video Processing</source>
        <translation>Обработка видео</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Available bandwidth:</source>
        <translation>Доступная ширина канала:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Use this field to simulate the maximum bandwidth available so as to preview what the encoded video will look like with the corresponding compression rate.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Воспользуйтесь этим полем для симуляции максимально доступной ширины канала, чтобы увидеть, как будет выглядеть видео после кодирования с выбранной степенью сжатия&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB/s</source>
        <translation>КБ/сек</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Display encoded (and then decoded) frame, to check the codec&apos;s quality. If not selected, the image above only shows the frame that is grabbed from your camera.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Отобразить кодированный (а затем декодированный) кадр, чтобы проверить качество кодеков. Если не выбрано, на изображении вверху будет показан кадр, полученный с вашей камеры.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>preview</source>
        <translation>предварительный просмотр</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="+202"/>
        <source>Continuous</source>
        <translation>непрерывный</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation>Голосовая активность</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation>Активация по кнопке</translation>
    </message>
    <message>
        <location line="+105"/>
        <source>%1 s</source>
        <translation>%1 с</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation>Выкл</translation>
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
        <translation>Аудио статистика</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation>Входящие уровни</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation>Пиковый уровень микрофона</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation>Пиковая мощность в последнем фрагменте</translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Показывает пиковую мощность последнего фрагмента (20 мс), и является тем же параметром, который Вы обычно видите как &quot;входящая мощность&quot;. Проигнорируйте этот параметр в пользу &lt;b&gt;Мощности микрофона&lt;/b&gt;, который является более корректным.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation>Пиковый уровень динамика</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Показывает пиковую мощность в последнем фрагменте (20 мс) динамиков. Пока Вы используете мультиканальный метод проб (такой как ASIO),настроенный на каналы динамика, она будет равна 0. Если у Вас они так сконфигурированы, все еще будет отображаться 0, пока Вы будете прослушивать звук из других программ, Ваши настройки не будут работать.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation>Пиковый уровень очистки</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Показывает пиковую мощность в последнем фрейме (20 мс) после всей обработки. В идеале здесь должно быть -96 дБ, когда Вы не говорите. В реальной жизни, в студии будет около -60 дБ, но Вы увидите в лучшем случае -20 дБ. Когда Вы говорите этот показатель будет увеличиваться в районе от -5 до -10 дБ.&lt;br /&gt;Если Вы используете подавление эхо, и это значение повышается более -15 дБ, когда Вы не говорите - Ваши настройки не работают и Вы будете раздражать других пользователей эхом.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation>Анализ сигнала</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation>Мощность микрофона</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation>Как близок текущий входящий уровень к идеалу</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Показывает насколько близок Ваш текущий уровень входящей громкости к идеалу. Чтобы настроить громкость микрофона, откройте любую программу настройки громкости, и настройте громкость записи, затем посмотрите это значение во время разговора. &lt;br /&gt;&lt;b&gt;Говорите громко, как будто расстроены или раздражены действиями нуба.&lt;/b&gt;&lt;br /&gt;Настраивайте громкость, до тех пор, пока число не будет близко к 100%, но убедитесь, что оно не поднимется выше. Если оно будет выше, скорее всего будут прерывания Вашей речи, что ухудшит качество звука.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Соотношение Сигнал/Шум</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Соотношение Сигнал/Шум от микрофона</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>Отношение Сигнал/Шум (SNR) микрофона в последнем фрагменте (20 мс). Показывает, насколько чист голос по сравнению с шумом.&lt;br /&gt;Если значение ниже 1.0, в сигнале больше шума, нежели голоса, и поэтому ухудшается качество.&lt;br /&gt;Верхнего предела не существует, но не ожидайте увидеть выше 40-50, если Вы не в студии звукозаписи.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation>Вероятность речи</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation>Вероятность речи</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Вероятность того, что последний фрагмент (20 мс) был речью, а не шумом.&lt;br /&gt;Передача голосовой активности зависит от верности этого. Фишка в том, что середина предложения всегда распознается как речь; проблема в паузах между словами и началом разговора. Трудно отделить кашель от слова, начинающегося на &apos;х&apos;.&lt;br /&gt;Если это выделено жирным шрифтом, значит RetroShare сейчас передает (если Вы подключены).</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation>Текущие настройки</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation>Текущий битрейт звука</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation>Битрейт последнего фрагмента</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>Аудио битрейт последнего сжатого фрагмента (20 мс), который будет меняться, как VBR (Variable Bit Rate - переменная скорость передачи). Пиковый битрейт может быть отрегулирован в Настройках.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation>Скорость двойного нажатия</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Время между последними двумя нажатиями Кнопки активации речи</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation>Определение речи</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation>Шанс определения речи</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Показывает текущие настройки определения речи.&lt;/b&gt;&lt;br /&gt;Вы можете изменить эти настройки в Настройках или в Мастере настройки звука.</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation>Спектр мощности сигнала и шума</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Мощностной спектр входящего сигнала и оценка шума</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Показывает мощностной спектр текущего входящего сигнала (красная линия) и текущая оценка шума (заполнена синим).&lt;br /&gt;Все амплитуды умножены на 30, чтобы показать интересные части (на сколько больше сигнала, чем шума представлено в каждом отрезке).&lt;br /&gt;Это только если Вы интересуетесь точными условиями шума вашего микрофона. При хороших условиях, это будет всего лишь крошечным синим отрезком внизу. Если синего на графике более половины, у Вас серьезные проблемы с шумом окружающей среды.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation>Анализ Эха</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation>Значение подавления эха</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Отображает значение подавления эха, со временем растущего вниз и увеличивающим частоту вправо.&lt;br /&gt;В идеале, все должно быть черным, отображая, что эха нет. В общем, у Вас будет 1 или несколько горизонтальных полосок синеватого цвета, отображающих время задержки эха. Вы должны видеть значения в реальном времени.&lt;br /&gt;Заметьте, что если у Вас нет эхо, которое нужно подавить, Вы не увидите здесь полезной информации. Запустите музыку, и все должно нормализоваться. &lt;br /&gt;Вы можете выбрать просмотр реальных или воображаемых частей доменно-частотных значений, или наоборот высчитанные модуль и фазу. Наиболее применимым из них будут, скорее всего модуль, который является амплитудой эха, и показывает, сколько сигнала было вырезано за промежуток времени. Другие режимы просмотра более полезны для людей, желающих настроить алгоритмы глушения эха.&lt;br /&gt;Пожалуйста, заметьте: Если картина в целом обширно варьируется в режиме модулей, подавление эха не может найти соответствия между двумя входными источниками (динамиками и микрофоном). Или у Вас очень долгая задержка эха, или один из входящих источников настроен неправильно.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Мастер настройки звука</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation>Вступление</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Добро пожаловать в мастер настройки звука RetroShare</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Вас приветствует мастер настройки звука RetroShare. Он поможет Вам правильно настроить Вашу звуковую карту, а также установить наилучшие параметры для обработки звука.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation>Настройка громкости</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Установите оптимальные настройки громкости микрофона.</translation>
    </message>
    <message>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="vanished">&lt;p&gt;Откройте системную панель управления звуком и перейдите к параметрам записи. Убедитесь, что микрофон выбран в качестве активного входа с максимальной громкостью записи. Если есть возможность включить &quot;усиление Микрофона&quot; (Microphone Boost) - включите его.&lt;/p&gt;
&lt;p&gt;Говорите громко, как будто Вы раздражены или взволнованы. Уменьшите громкость звука в системной панели управления так, чтобы показания индикатора звука были максимум в синей и зеленой зоне, но &lt;b&gt;НЕ&lt;/b&gt; красной, пока Вы говорите.&lt;/p&gt;</translation>
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
        <translation>Говорите нормально и переместите ползунок ниже, так что бы бар переходил в зеленую зону, когда вы говорите и не появлялся в оранжевой зоне.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation>Остановить цикл эхо для этого мастера</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Применить высококонтрастную цветовую схему для людей со слабым зрением</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation>Использовать высококонтраструю цветовую схему</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation>Определение Голосовой активности</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>Укажите RetroShare когда Вы говорите и когда Вы молчите.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Этот шаг поможет RetroShare определить, когда Вы говорите. Сначала выберите - какие данные для этого использовать.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation>Активация по кнопке:</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation>Обнаружение Голоса</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>Затем Вы должны откорректировать бегунок ниже. С начала произнесения речи индикатор НЕ должен выходить на зеленую зону (Ваша речь). Когда Вы продолжаете говорить, индикатор должен остаться в желтой зоне (могла бы быть Ваша речь) и когда Вы молчите, индикатор должен оставаться в красной зоне (не речь).</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation>Непрерывная передача</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation>Закончить</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation>Наслаждайтесь использованием RetroShare</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Поздравляем. Теперь Вы готовы насладиться более богатым звуком с RetroShare.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="+128"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;RetroShare VOIP плагин&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="vanished">&lt;br/&gt;Плагин VOIP добавляет VOIP в частном окне чата из RetroShare. Чтобы использовать его, выполните следующие действия:&lt;UL&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt;установка уровня использования микрофона с помощью панели настройки&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation type="vanished">&lt;li&gt;проверьте свой ​​микрофон, глядя на VU-Меттерс&lt;/li&gt;</translation>
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
        <translation>&lt;li&gt; в личном чате, включите звук нажимая на две клавиши VOIP&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>Ваш друг должен активировать данный плагин, чтобы иметь возможность общаться с вами посредством голосовой и/или видеосвязи.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;Это экспериментальная функция. Не стесняйтесь,  отправить комментарии и предложения для команды разработчиков RS.</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location line="+47"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>Плагин обеспечивает голосовую и видеосвязь между доверенными узлами в сети RetroShare.</translation>
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
        <translation>Входящий аудиозвонок</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Incoming video call</source>
        <translation>Входящий видеозвонок</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing audio call</source>
        <translation>Исходящий аудиозвонок</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing video call</source>
        <translation>Исходящий видеозвонок</translation>
    </message>
</context>
<context>
    <name>VOIPChatWidgetHolder</name>
    <message>
        <location filename="../gui/VOIPChatWidgetHolder.cpp" line="+70"/>
        <location line="+146"/>
        <source>Mute</source>
        <translation>Отключить звук</translation>
    </message>
    <message>
        <location line="-128"/>
        <location line="+138"/>
        <source>Start Call</source>
        <translation>Позвонить</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Start Video Call</source>
        <translation>Совершить видеозвонок</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Hangup Call</source>
        <translation>Удерживать вызов</translation>
    </message>
    <message>
        <location line="-113"/>
        <location line="+626"/>
        <source>Hide Chat Text</source>
        <translation>Скрыть текст чата</translation>
    </message>
    <message>
        <location line="-608"/>
        <location line="+106"/>
        <location line="+523"/>
        <source>Fullscreen mode</source>
        <translation>Полноэкранный режим</translation>
    </message>
    <message>
        <source>%1 inviting you to start an audio conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 приглашает вас начать голосовое общение. Хотите Принять или Отклонить приглашение?</translation>
    </message>
    <message>
        <location line="-410"/>
        <source>Accept Audio Call</source>
        <translation>Принять аудиозвонок</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Decline Audio Call</source>
        <translation>Отклонить аудиозвонок</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse audio call</source>
        <translation>Отклонить аудиовызов</translation>
    </message>
    <message>
        <source>%1 inviting you to start a video conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 приглашает вас начать видеообщение. Хотите Принять или Отклонить приглашение? </translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Decline Video Call</source>
        <translation>Отклонить Видеовызов</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse video call</source>
        <translation>Отменить видеозвонок</translation>
    </message>
    <message>
        <location line="+102"/>
        <source>Mute yourself</source>
        <translation>Выключить микрофон</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmute yourself</source>
        <translation>Включить микрофон</translation>
    </message>
    <message>
        <source>Waiting your friend respond your video call.</source>
        <translation type="vanished">Ожидание ответа друга на ваш видео звонок</translation>
    </message>
    <message>
        <location line="+603"/>
        <source>Your friend is calling you for video. Respond.</source>
        <translation>Ваш друг пытается сделать видеозвонок вам. Принять.</translation>
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
        <translation>Статус VoIP</translation>
    </message>
    <message>
        <location line="-467"/>
        <source>Hold Call</source>
        <translation>Удержание Вызова</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Outgoing Call is started...</source>
        <translation>Начинаем исходящий вызов...</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Resume Call</source>
        <translation>Возобновить вызов</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Outgoing Audio Call stopped.</source>
        <translation>Исходящий аудиовызов прерван.</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Shut camera off</source>
        <translation>Выключить камеру</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>You&apos;re now sending video...</source>
        <translation>Сейчас вы отправляете видео...</translation>
    </message>
    <message>
        <location line="-266"/>
        <location line="+279"/>
        <source>Activate camera</source>
        <translation>Включить камеру</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video call stopped</source>
        <translation>Видеозвонок завершён</translation>
    </message>
    <message>
        <location line="-295"/>
        <source>Accept Video Call</source>
        <translation>Принимать видеозвонок</translation>
    </message>
    <message>
        <location line="-55"/>
        <source>%1 is inviting you to start an audio conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activate audio</source>
        <translation>Активировать аудио</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>%1 is inviting you to start a video conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+334"/>
        <source>Show Chat Text</source>
        <translation>Показать содержание чата</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Return to normal view.</source>
        <translation>Вернуть обычный вид</translation>
    </message>
    <message>
        <location line="+228"/>
        <source>%1 hang up. Your call is closed.</source>
        <translation>%1 отключился. Ваш вызов завершён.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your audio call is closed.</source>
        <translation>%1 отключился. Ваш аудиозвонок завершён.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your video call is closed.</source>
        <translation>%1 отключился. Ваш видеозвонок завершён.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>%1 accepted your audio call.</source>
        <translation>%1 принял ваш аудиозвонок.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 accepted your video call.</source>
        <translation>%1 принял ваш видеозвонок.</translation>
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
        <translation type="vanished">Ожидание ответа друга на ваш вызов.</translation>
    </message>
    <message>
        <location line="-44"/>
        <source>Your friend is calling you for audio. Respond.</source>
        <translation>Ваш друг совершает аудиовызов. Ответьте.</translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+58"/>
        <source>Answer</source>
        <translation>Ответить</translation>
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
        <translation>Ответить</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Answer with video</source>
        <translation>Ответить с видео</translation>
    </message>
    <message>
        <location filename="../gui/VOIPToasterItem.ui" line="+232"/>
        <source>Decline</source>
        <translation>Отклонить</translation>
    </message>
</context>
<context>
    <name>VOIPToasterNotify</name>
    <message>
        <location filename="../gui/VOIPToasterNotify.cpp" line="+56"/>
        <source>VOIP</source>
        <translation>VoIP</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Accept</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bandwidth Information</source>
        <translation>Информация о трафике</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Audio or Video Data</source>
        <translation>Аудио или видео данные</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>HangUp</source>
        <translation>Повесить Трубку</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Invitation</source>
        <translation>Приглашение</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Audio Call</source>
        <translation>Аудио вызов</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Video Call</source>
        <translation>Видео звонок</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Test VOIP Accept</source>
        <translation>Принять тест VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP BandwidthInfo</source>
        <translation>Тест трафика VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Data</source>
        <translation>Тест данных VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP HangUp</source>
        <translation>Тест VOIP повесить трубку </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Invitation</source>
        <translation>Тест VOIP приглашение</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Test VOIP Audio Call</source>
        <translation>Тест VOIP аудио звонок</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Video Call</source>
        <translation>Тест VOIP видео звонок</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Accept received from this peer.</source>
        <translation>Одобрите получение от этого участника.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Bandwidth Info received from this peer: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bandwidth Info received from this peer:%1</source>
        <translation type="vanished">Информация о трафике получена от этого участника: % 1</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Audio or Video Data received from this peer.</source>
        <translation>Аудио и видео данные получены от этого участника.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>HangUp received from this peer.</source>
        <translation>Отбой звонка получен от этого участника.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Invitation received from this peer.</source>
        <translation>Приглашение получено от этого участника.</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+24"/>
        <source>calling</source>
        <translation>вызов</translation>
    </message>
</context>
<context>
    <name>voipGraphSource</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="-260"/>
        <source>Required bandwidth</source>
        <translation>Требуемая ширина полосы пропускания</translation>
    </message>
</context>
</TS>
