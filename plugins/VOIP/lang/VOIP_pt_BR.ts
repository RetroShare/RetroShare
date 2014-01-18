<?xml version="1.0" ?><!DOCTYPE TS><TS language="pt_BR" version="2.0">
<context>
    <name>AudioChatWidgetHolder</name>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="27"/>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="95"/>
        <source>Mute yourself</source>
        <translation>Deixar Mudo</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="47"/>
        <source>Start Call</source>
        <translation>Iniciar chamada</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="69"/>
        <source>Hangup Call</source>
        <translation>Desligar chamada</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="97"/>
        <source>Unmute yourself</source>
        <translation>Retirar mudo</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="126"/>
        <source>Hold Call</source>
        <translation>Colocar chamada em espera</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="143"/>
        <source>VoIP Status</source>
        <translation>Estado do VoIP</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="143"/>
        <source>Outgoing Call is started...</source>
        <translation>Chamada de saída foi iniciada...</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="151"/>
        <source>Resume Call</source>
        <translation>Continuar chamada</translation>
    </message>
    <message>
        <location filename="../gui/AudioChatWidgetHolder.cpp" line="175"/>
        <source>Answer</source>
        <translation>Responder</translation>
    </message>
</context>
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="17"/>
        <source>Audio Wizard</source>
        <translation>Assistente de áudio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="30"/>
        <source>Transmission</source>
        <translation>Transmissão</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="36"/>
        <source>&amp;Transmit</source>
        <translation>&amp;Transmitir</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="46"/>
        <source>When to transmit your speech</source>
        <translation>Quando transmitir sua fala</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="49"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;Define quando sua fala deve ser transmitida.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Contínua&lt;/i&gt; - Todo o tempo&lt;br /&gt;&lt;i&gt;Atividade vocal&lt;/i&gt; - Quando você fala claramente.&lt;br /&gt;&lt;i&gt;Pressionar Para Falar&lt;/i&gt; - Quando você pressionar a tecla de atalho escolhida em &lt;i&gt;Atalhos&lt;/i&gt;.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="63"/>
        <source>DoublePush Time</source>
        <translation>Tempo para DuploClique</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="73"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Se pressionar a tecla PPF duas vezes neste tempo ela ficará travada.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="76"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;Tempo para DuploClique&lt;/b&gt;&lt;br /&gt;Se você pressionar a tecla para falar duas vezes durante o intervalo de tempo configurado ela ficará travada. O RetroShare ocnitnuará transmitindo até que você  pressione-a uma vez mais para destravar o PPF de novo.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="119"/>
        <source>Voice &amp;Hold</source>
        <translation>&amp;Persistência de voz</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="129"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Quanto tempo continuar a transmissão após silêncio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="132"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Seleciona quanto tempo após uma perceptível parada na fala a transmissão deve continuar.&lt;/b&gt;&lt;br /&gt;Configure isto mais alto se sua voz quebra durante sua fala (percebido pelo ícone de voz piscando ao lado de seu nome).</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="148"/>
        <source>Silence Below</source>
        <translation>Silêncio abaixo</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="155"/>
        <source>Signal values below this count as silence</source>
        <translation>Valores de sinal abaixo disto contam como silêncio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="158"/>
        <location filename="../gui/AudioInputConfig.ui" line="190"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Estabaelece os gatilhos para a detecção de voz.&lt;/b&gt;&lt;br /&gt;Use isto junto com a janela de Estatísticas de Áudio para afinar manualmente os valores para o gatilho de deteção de voz. Valores de entrada baixo de &quot;Silêncio abaixo&quot; sempre contam como silêncio. Valores acima de &quot;Fala acima&quot; sempre contam como voz. Valores entre ambos contam como voz se você já está falando, mas não levam a uma nova detação.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="180"/>
        <source>Speech Above</source>
        <translation>Fala acima</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="187"/>
        <source>Signal values above this count as voice</source>
        <translation>Sinal acima disto conta como voz</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="225"/>
        <source>empty</source>
        <translation>vazio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="240"/>
        <source>Audio Processing</source>
        <translation>Processamento de áudio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="246"/>
        <source>Noise Suppression</source>
        <translation>Supressão de Ruídos</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="259"/>
        <source>Noise suppression</source>
        <translation>Supressão de ruídos</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="262"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Configura a quantia de supressão de ruído aplicada.&lt;/b&gt;&lt;br /&gt;Quão maior este valor, mais agressivamente o ruído estacionário será suprimido.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="294"/>
        <source>Amplification</source>
        <translation>Amplificação</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="304"/>
        <source>Maximum amplification of input sound</source>
        <translation>Amplificação máxima do som de entrada</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="307"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;Amplificação máxima da entrada.&lt;/b&gt;&lt;br /&gt;O RetroShare normaliza o volume de entrada antes da compressão, e isto configura quando ele pode amplificar.&lt;br /&gt;O nível real é continuamente atualizado baseado no seu padrão atual de fala, mas nunca passará do especificado aqui.&lt;br /&gt;Se a &lt;i&gt;Altura do microfone&lt;/i&gt; nível das estatísticas de áudio ficam sobre os 100%, você provavelmente quer configurar isto para 2.0 ou parecido, mas se , como a maioria das pessoas, você é incapaz de chegar aos 100%, configure isto muito mais alto.&lt;br /&gt;Idealmente, configure dem odo que a &lt;i&gt;Altura do microfone * Factor de Amplificação &gt;= 100&lt;/i&gt;, mesmo quando você está falando baixo.&lt;br /&gt;&lt;br /&gt;Note que não há dano em configurar isto no máximo, mas o RetroShare começará a transmitir outras conversar se você deixá-lo auto-afinado para este nível.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="339"/>
        <source>Echo Cancellation Processing</source>
        <translation>Processo Cancelamento de Echo</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="100"/>
        <source>Continuous</source>
        <translation>Contínuo</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="101"/>
        <source>Voice Activity</source>
        <translation>Atividade de Voz</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="102"/>
        <source>Push To Talk</source>
        <translation>Push To Talk</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="204"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="212"/>
        <source>Off</source>
        <translation>Off</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="215"/>
        <source>-%1 dB</source>
        <translation>-%1 dB</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="72"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="14"/>
        <source>Audio Statistics</source>
        <translation>Estatísticas de Áudio</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="22"/>
        <source>Input Levels</source>
        <translation>Níveis de Entrada</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="28"/>
        <source>Peak microphone level</source>
        <translation>Nível máximo do microfone</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="35"/>
        <location filename="../gui/AudioStats.ui" line="55"/>
        <location filename="../gui/AudioStats.ui" line="75"/>
        <source>Peak power in last frame</source>
        <translation>Potência máxima no último quadro</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="38"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Mostra a potência máxima no último quadro (20 ms), e é a mesma medida que você geralmente encontra como &quot;potência de entrada&quot;. Por favor desconsidere isto e veja &lt;b&gt;Potência do microfone&lt;/b&gt;, que é muito mais regular e desconsidera extremos.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="48"/>
        <source>Peak speaker level</source>
        <translation>Nível máximo de alto-falante</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="58"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Mostra o pico de potência dos alto-falantes no último quadro (20ms). A menos que você esteja usando um método de amostra multi-canal (como ASIO) com alto-falantes configurados, isto será 0. Se você possui tal instalação configurada, e isto ainda mostra 0 enquanto você está tocando áudio de outros programas, sua instalação não está funcionando.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="68"/>
        <source>Peak clean level</source>
        <translation>Nível limpo máximo</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="78"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Mostra a potência máxima no último quadro (20ms) depois de todo processamento. Idealmente, isto deve ser -96 dB quando você não está falando. Na realidade, um estúdio sonoro deve ver -60 dB, e com sorte você deve ver algo ao redor de -20 dB. Quando você está falando, isto deve aumentar para algo em torno de -5 e -10 dB.&lt;br /&gt;Se você está usando anulação de eco, e isto aumenta para mais de -15 dB quando você não está falando, sua configuração não está funcionando, e você vai incomodar outros usuários com ecos.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="91"/>
        <source>Signal Analysis</source>
        <translation>Análise de sinal</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="97"/>
        <source>Microphone power</source>
        <translation>Potência do microfone</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="104"/>
        <source>How close the current input level is to ideal</source>
        <translation>Quão próximo o nível de entrada atual está do ideal</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="107"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Mostra quão perto seu volume de entrada atual está do ideal. Para ajustar o nível do seu microfone, abra seja qual for o programa que você usa para configurar o volume de gravação, e olha para o valor enquanto fala.&lt;br /&gt;&lt;b&gt;Fale alto, como você faria quando você está incomodado por ser morto por um novato.&lt;/b&gt;&lt;br /&gt;Ajuste o volume até que este valor esteja próximo a 100%, mas certifique-se que ele não vá acima, é provável que você consiga cliques em partes da sua fala que vão degradar a qualidade do som.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="117"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Relação sinal/ruído</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="124"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Taxa sinal/ruído do microfone</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="127"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>Esta é a relação sinal/ruído (SNR) do microfone no último quadro (20 ms). Ele mostra quão mais clara a voz é compara com o ruído.&lt;br /&gt;Se este valor está abaixo de 1.0, há muito mais ruído do que voz no sinal, e então a qualidade é reduzida.&lt;br /&gt;Não há um limite superior para este valor, mas não espereve ver muito acima de 40-50 sem um estúdio de som.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="137"/>
        <source>Speech Probability</source>
        <translation>Probabilidade de fala</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="144"/>
        <source>Probability of speech</source>
        <translation>Provavilidade de fala</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="147"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Este é provavelmetne o último quadro (20 ms) que era fala e não ruído ambiente.&lt;br /&gt;Transmissão da atividade vocal depende disto estar certo. O truque com isto é que o meio da sentença é sempre detectado como fala; o problema são as pausas entre as palavras e o começo da fala. É difícil distinguir um suspiro de uma palavra começando com &apos;h&apos;.&lt;br /&gt;Se isto está em negrito, significao que o RetroShare está transmitindo atualmente (se você está conectado).</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="162"/>
        <source>Configuration feedback</source>
        <translation>Retroalimentação de configuração</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="168"/>
        <source>Current audio bitrate</source>
        <translation>Velocidade atual de transmissão de áudio</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="181"/>
        <source>Bitrate of last frame</source>
        <translation>Velocidade do último quadro</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="184"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>Esta é a taxa de áudio para o último quadro comprimido (20 ms), e como tal irá pular para cima e para baixo conforma o VBD ajusta a qualidade. O pico da taxa pode ser ajustado no diálogo de Configuração.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="194"/>
        <source>DoublePush interval</source>
        <translation>Intervalo de DuploClique</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="207"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Tempo entra os dois últimos pressionamentos de Pressionar-para-Falar</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="217"/>
        <source>Speech Detection</source>
        <translation>Deteção de fala</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="224"/>
        <source>Current speech detection chance</source>
        <translation>Probabilidade atual de deteção de fala</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="227"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Mostra as configurações atuals de deteção de fala&lt;/b&gt;&lt;br /&gt;Você pode mudar as configurações do diálogo d Configurações ou do Assistente de áudio.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="256"/>
        <source>Signal and noise power spectrum</source>
        <translation>Espéctro de potência de sinal e ruído</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="262"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Espéctro de potência do sinal de entrada e estimativa de ruído</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="265"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Mostra o espctro da potência do sinal de entrada atual (linha vermelha) e a estimativa de ruído atual (azul preenchido).&lt;br /&gt;Todas amplitudes são multiplicadas por 30 para mostrar as partes interessantes (quão mais sinal do que ruído está presente em cada banda de onda).&lt;br /&gt;Provavelmente isto só é de interesse se você está tentando afinar condições de ruído no seu microfone. Sob boas condições, deve haver apenas uma pequena agitação de azul no fundo. Se o azul é mais do que metade do gráfico, você tem um ambiente bem ruidoso.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="281"/>
        <source>Echo Analysis</source>
        <translation>Análise de eco</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="293"/>
        <source>Weights of the echo canceller</source>
        <translation>Pesos do anulador de eco</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="296"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Mostra os pesos do cancelador de ecos, com o tempo aumentando para baixo e a frequencia para a direita.&lt;br /&gt;Idealmente, isto deve ser preto, indicando que nenhum eco existe. Mais comumente, você verá uma ou mais listras horizontais de tom azul represetando eco com tempo atrazado. Você deve ser capaz de ver os pesos atualizados em tempo real.&lt;br /&gt;Por favor note que enquanto você não tiver nada para ecoar, você não verá muitos dados úteis aqui. Toque alguma música e as coisas devem estabilizar.&lt;br /&gt;Você pode escolher ver as partes reais ou imaginárias dos pesos no domínio da frequência, ou alternativamente a fase e o módulo calculados. Os mais úteis destes provavelmente serão o módulo, que é a amplitude do eco, e mostra quanto do sinal transmitido está sendo eliminado naquela etapa no tempo. Os outros modos de visão são mais úteis para pessoas que querem melhorar os algoritmos de anulação de echo.&lt;br /&gt;Por favor note: Se a imagem inteira flutua massivamente no modo de módulo, o anulador de eco falha ao encontrar qualquer correlação entre as duas fontes de entrada (alto-falantes e microfone). Ou você possui um atrazo muito longo no eco, ou as fontes de entrada estão configuradas errado.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Assistente para ajuste do áudio</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="18"/>
        <source>Introduction</source>
        <translation>Introdução</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="21"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Bem-vindo ao assistente para ajuste do áudio do RetroShare</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="32"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Este é o assistente para ajuste de áudio do RetroShare. Ele vai ajudá-lo a configurar corretamente os níveis de entrada para sua placa de som, e também os parâmetros para processamento de som no RetroShare.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="56"/>
        <source>Volume tuning</source>
        <translation>Ajusta de volume</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="59"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Ajustando o volume do hardware para níveis ideais.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="70"/>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there's an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it's checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation>&lt;p&gt;Abra seus painel de controle de som e vá para as configurações de gravação. Tenha certeza de que o microfone está marcado como ativo e com o máximo de volume. Se houver uma opção para ativar o &quot;Aumento de sensibilidade&quot;, certifique-se de marcá-la.&lt;/p&gt;
&lt;p&gt;Fale alto, como quando você está incomodado ou animado. Diminua o volume no painel de controle até que a barra baixo fique o mais alto possível no azul e verde, mas &lt;b&gt;não&lt;/b&gt; na zona vermelha enquanto você fala.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="86"/>
        <source>Talk normally, and adjust the slider below so that the bar moves into green when you talk, and doesn&apos;t go into the orange zone.</source>
        <translation>Fale normalmente, e ajuste at</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="130"/>
        <source>Stop looping echo for this wizard</source>
        <translation>Para looping echo para este wizard</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="150"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Aplicar algumas otimizações de alto contraste para usuários com deficiência visual</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="153"/>
        <source>Use high contrast graphics</source>
        <translation>Usar gráficos de alto cotnraste</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="163"/>
        <source>Voice Activity Detection</source>
        <translation>Deteção de atividade de voz</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="166"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>Permitir que o RetroShare suponha quando você está falando e quando está calado.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="172"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Ajuda o RetroShare a supor quando você fala. O primeiro passo é selecionar que valor de dados a usar.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="184"/>
        <source>Push To Talk:</source>
        <translation>Pressionar Para Falar:</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="213"/>
        <source>Voice Detection</source>
        <translation>Detecção de voz</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="226"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>A seguir você precisa ajusar a seguinte barra de rolagem. As primeiras palavras que você falar devem chegar a área verde (definitivamente fala). Enquanto você fala você deve ficar dentro do amarelo (pode ser fala) e quando você não estivar falando tudo deve ficar no vermelho (definitivamente não é fala).</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="290"/>
        <source>Continuous transmission</source>
        <translation>A transmissão contínua</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="298"/>
        <source>Finished</source>
        <translation>Finalizado</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="301"/>
        <source>Enjoy using RetroShare</source>
        <translation>Disfrute do uso do RetroShare</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="312"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Congratulations. You should now be ready to enjoy a richer sound experience with RetroShare.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="95"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt; * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="96"/>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation>&lt;br/&gt;O VOIP plugin adiciona  VOIP no chat privado do RetroShare. Para usar, siga as instruções:&lt;UL&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="97"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt; configure o volume do microfone usando o painel de configuração&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="98"/>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation>&lt;li&gt; check o seu microfone usando VU-metters&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="99"/>
        <source>&lt;li&gt; in the private chat, enable sound input/output by clicking on the two VOIP icons&lt;/li&gt;&lt;/ul&gt;</source>
        <translation>&lt;li&gt; no chat privado, habilite o som entrada/saída clickando nos dois ícones VOIP&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="100"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>Seu amigo precisa usar o plugin para escutar/falar com você, é claro.</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="101"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="126"/>
        <source>RTT Statistics</source>
        <translation>Estatísticas RTT</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="145"/>
        <location filename="../gui/VoipStatistics.cpp" line="147"/>
        <location filename="../gui/VoipStatistics.cpp" line="149"/>
        <source>secs</source>
        <translation>secs</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="151"/>
        <source>Old</source>
        <translation>velho</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="152"/>
        <source>Now</source>
        <translation>agora</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="361"/>
        <source>Round Trip Time:</source>
        <translation>Round Trip Time:</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="163"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>Este plugin proporciona comunicação de voz entre amigos no RetroShare.</translation>
    </message>
</context>
<context>
    <name>VOIPPlugin</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="168"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>VoipStatistics</name>
    <message>
        <location filename="../gui/VoipStatistics.ui" line="14"/>
        <source>VoipTest Statistics</source>
        <translation>Estatísticas VoipTest</translation>
    </message>
</context>
</TS>