<?xml version="1.0" ?><!DOCTYPE TS><TS language="it" version="2.0">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="17"/>
        <source>Audio Wizard</source>
        <translation>Autoconfiguratore audio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="30"/>
        <source>Transmission</source>
        <translation>Trasmissione</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="36"/>
        <source>&amp;Transmit</source>
        <translation>&amp;Trasmetti</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="46"/>
        <source>When to transmit your speech</source>
        <translation>Quando trasmettere la tua voce</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="49"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;Imposta quando la voce dovrebbe essere trasmessa.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuamente&lt;/i&gt; - Tutto il tempo&lt;br /&gt;&lt;i&gt;Attivazione vocale&lt;/i&gt; - Quando stai parlando.&lt;br /&gt;&lt;i&gt;Premi-Per-Parlare&lt;/i&gt; - Quando premi il pulsante PPP (Premi Per Parlare) impostato sotto &lt;i&gt;Scorciatoie&lt;/i&gt; nelle impostazioni di Mumble.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="63"/>
        <source>DoublePush Time</source>
        <translation>Tempo di DoppiaPressione</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="73"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Se premi il pulsante PPP due volte la trasmissione si dovrebbe bloccare.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="76"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt; Tempo di DoublePush&lt;/ b&gt;&lt;br /&gt; Se si preme  il tasto push-to-talk (premi-e-parla) due volte durante l&apos;intervallo di tempo configurato, esso resterà bloccato. RetroShare continuerà a trasmettere fino a quando si preme il tasto ancora una volta per sbloccare nuovamente PTT.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="119"/>
        <source>Voice &amp;Hold</source>
        <translation>Trasmetti &amp;per</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="129"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Per quanto tempo continua a trasmettere dopo l&apos;inizio del silenzio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="132"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Imposta per quanto tempo dopo la fine del discorso la trasmissione deve continuare.&lt;/b&gt;&lt;br /&gt;Aumentalo se la tua voce viene tagliata mentre stai parlando (si vedi un lampeggio rapido dell&apos;icona voce accanto al tuo nome).</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="148"/>
        <source>Silence Below</source>
        <translation>Sottofondo</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="155"/>
        <source>Signal values below this count as silence</source>
        <translation>Valore del segnale sotto il quale l&apos;ngresso conta come silenzio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="158"/>
        <location filename="../gui/AudioInputConfig.ui" line="190"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Questo cursore imposta il valore di soglia per il rilevamento del discorso.&lt;/b&gt;&lt;br /&gt;Usa questo insieme alla finestra delle statistiche audio per sintonizzare manualmente il cursore per i valori di soglia per l&apos;individuazione del discorso. un segnale al di sotto del cursore &quot;Sottofondo&quot; (zona rossa) viene sempre considerato come silenzio. Un segnale al di sopra del cursore &quot;Voce&quot; (zona gialla) viene considerato come discorso. Un segnale compreso tra i due cursori conterà come discorso solo se prima il valore  del segnale in ingresso è andato nella zona verde, ma non farà iniziare una nuova trasmissione.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="180"/>
        <source>Speech Above</source>
        <translation>Voce</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="187"/>
        <source>Signal values above this count as voice</source>
        <translation>Valore del segnale sopra il quale l&apos;ingresso conta come discorso</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="225"/>
        <source>empty</source>
        <translation>vuoto</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="240"/>
        <source>Audio Processing</source>
        <translation>Elaborazione Audio</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="246"/>
        <source>Noise Suppression</source>
        <translation>Riduzione Rumore</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="259"/>
        <source>Noise suppression</source>
        <translation>Riduzione rumore</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="262"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Questo imposta la quantità di rumore da eliminare.&lt;b&gt;&lt;br /&gt;Più alto sarà questo valore e più rumore fisso sarà eliminato.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="294"/>
        <source>Amplification</source>
        <translation>Amplificazione</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="304"/>
        <source>Maximum amplification of input sound</source>
        <translation>Massima amplificazione del segnale di ingresso</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="307"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;br&gt;Amplificazione massima di ingresso.&lt;/ b&gt;&lt;br /&gt; RetroShare normalizza il volume di ingresso prima della compressione, e questo regola quanto è permesso amplificare. &lt;br /&gt;Il livello attuale viene continuamente aggiornato in base al tuo modello vocale corrente, ma non potrà mai andare al di sopra del livello qui specificato.&lt;br /&gt; Se il &lt;i&gt;Volume del microfono&lt;/ i&gt; delle statistiche audio si aggira intorno al 100%, probabilmente vorrai impostare questo valore a 2.0 o giù di lì, ma se, come la maggior parte delle persone, non riesci a raggiungere il 100%, impostalo a un livello un po&apos; più alto. &lt;br /&gt; Idealmente, imposta così &lt;i&gt;Volume microfono * Fattore di Amplificazione &gt; = 100 &lt;/ i&gt;, anche quando parli sottovoce. &lt;br /&gt;&lt;br /&gt; Nota  che non vi è nulla di male ad impostare questo al massimo, ma RetroShare  inizia a preferirre altre conversazioni, se si lascia per l&apos;auto-regolazione a quel livello.</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="339"/>
        <source>Echo Cancellation Processing</source>
        <translation>Elaborazione cancellazione dell&apos;eco</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="98"/>
        <source>Continuous</source>
        <translation>Continuo</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="99"/>
        <source>Voice Activity</source>
        <translation>Attività Voce</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="100"/>
        <source>Push To Talk</source>
        <translation>Premi Per Parlare</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="202"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="210"/>
        <source>Off</source>
        <translation>Spento</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="213"/>
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
    <name>AudioPopupChatDialog</name>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="15"/>
        <source>Mute yourself</source>
        <translation>Mettiti in muto</translation>
    </message>
    <message>
        <location filename="../gui/AudioPopupChatDialog.cpp" line="35"/>
        <source>Deafen yourself</source>
        <translation>Azzera l&apos;audio</translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="14"/>
        <source>Audio Statistics</source>
        <translation>Statistiche Audio</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="22"/>
        <source>Input Levels</source>
        <translation>Livelli di Ingresso</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="28"/>
        <source>Peak microphone level</source>
        <translation>Livello di picco del microfono</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="35"/>
        <location filename="../gui/AudioStats.ui" line="55"/>
        <location filename="../gui/AudioStats.ui" line="75"/>
        <source>Peak power in last frame</source>
        <translation>Picco di potenza nell&apos;ultima parte</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="38"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Mostra la potenza di picco nell&apos;ultima parte (20 ms), ed è la stessa misurazione che si trova solitamente visualizzata come &quot;potenza in ingresso&quot;. Si prega di ignorare questo aspetto e invece guardare &lt;b&gt;Potenza del microfono&lt;/b&gt;, che è molto più costante e non tiene conto delle anomalie.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="48"/>
        <source>Peak speaker level</source>
        <translation>Livello di picco dell&apos;uscita</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="58"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Questo mostra la potenza di picco dell&apos;uscita nel corso delle ultime parti (20 ms). A meno che non si sta utilizzando un metodo di campionamento multi-canale (come ASIO), questo sarà 0. Se avete configurato una tale impostazione, e questo mostra ancora 0 mentre si sta riproducendo l&apos;audio da altri programmi, la configurazione non funziona.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="68"/>
        <source>Peak clean level</source>
        <translation>Livello pulito di picco</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="78"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Mostra la potenza di picco nell&apos;ultima parte (20 ms) dopo tutti i processi. Idealmente, questa dovrebbe essere -96 dB quando non stai parlando. In realtà, uno studio audio dovrebbe vedere -60 dB, e si spera che tu veda qualcosa intorno a -20 dB. Mentre stai parlando, questo dovrebbe salire tra -5 e -10 dB.&lt;br /&gt;Se stai usando la cancellazione dell&apos;eco, ed è piú alta di -15 dB quando non stai parlando, l&apos;impostazione non funziona, e infastidirai gli altri giocatori con l&apos;eco.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="91"/>
        <source>Signal Analysis</source>
        <translation>Analisi del segnale</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="97"/>
        <source>Microphone power</source>
        <translation>Potenza del microfono</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="104"/>
        <source>How close the current input level is to ideal</source>
        <translation>Quanto il livello dell&apos;ingresso attuale è vicino a quello ideale</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="107"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Mostra quanto il livello dell&apos;ingresso attuale è vicino a quello ideale. Per regolare il livello del microfono, apri il programma che usi per regolare il volume di registrazione, e guarda il valore mentre stai parlando.&lt;br /&gt;&lt;b&gt;Parla forte, come faresti quando sei spaventato.&lt;/b&gt;&lt;br /&gt;Regola il volume fino a quando questo valore è vicino al 100%, ma assicurarsi che non vada al di sopra. Se andesse sopra, probabilmente taglieresti alcune parti del tuo parlato, che degraderà la qualità del suono.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="117"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Rapporto Segnale/Rumore</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="124"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Rapporto Segnale/Rumore dal microfono</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="127"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>E&apos; il rapporto segnale/rumore (SNR) del microfono nell&apos;ultima parte (20 ms). Mostra quanto è piú forte la voce confrontata con il rumore.&lt;br /&gt;Se questo valore è inferiore a 1.0, c&apos;è piú rumore della voce nel segnale, e quindi la qualità sarà bassa.&lt;br /&gt;Non vi è alcun limite a questo valore, ma non aspettatevi di vederlo molto al di sopra di 40-50 senza essere in uno studio audio.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="137"/>
        <source>Speech Probability</source>
        <translation>Probabilità del parlato</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="144"/>
        <source>Probability of speech</source>
        <translation>Probabilità del parlato</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="147"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Ciò è la probabilità che l&apos;ultimo frame (20 ms) era voce e non rumore ambiente. &lt;br /&gt; L&apos;attività di Trasmissione Voice  dipende dall&apos;esattezza di questo. Il trucco sta&apos; che la metà di una frase è sempre rilevato come discorso, il problema sono le pause tra le parole e l&apos;inizio del discorso. E &apos;difficile distinguere un sospiro da una parola che inizia con &apos; H aspirata&apos;. &lt;br /&gt; Se questo è in grassetto, significa che RetroShare sta trasmettendo (se sei connesso).</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="162"/>
        <source>Configuration feedback</source>
        <translation>Ritorno di configurazione</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="168"/>
        <source>Current audio bitrate</source>
        <translation>Banda audio attuale</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="181"/>
        <source>Bitrate of last frame</source>
        <translation>Banda dell&apos;ultima parte</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="184"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>E&apos; il bitrate audio dell&apos;ultima parte compressa (20 ms), e come tale, si muove su e giú mentre il VBR regola la qualità. Puoi regolare il bitrate di picco attraverso la finestra delle Impostazioni.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="194"/>
        <source>DoublePush interval</source>
        <translation>Intervallo di DoppiaPressione</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="207"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Tempo tra le ultime due pressioni del tasto Premi-Per-Parlare</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="217"/>
        <source>Speech Detection</source>
        <translation>Riconoscimento del parlato</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="224"/>
        <source>Current speech detection chance</source>
        <translation>Probabilità attuale di riconoscimento del parlato</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="227"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Mostra l&apos;attuale intervento di rilevamento del parlato.&lt;/b&gt;&lt;br /&gt;Puoi cambiare le impostazioni della finestra di dialogo Impostazioni o dall&apos;Audio Wizard.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="256"/>
        <source>Signal and noise power spectrum</source>
        <translation>Potenza spettrale del segnale e del rumore</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="262"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Potenza spettrale del segnale e del rumore di ingresso stimata</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="265"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Mostra la potenza dello spettro del segnale di ingresso attuale (linea rossa) e l&apos;attuale stima del rumore (area di colore blu).&lt;br /&gt;Tutte le ampiezze sono moltiplicate per 30 per mostrare le parti interessanti (quanto piú il segnale rispetto al rumore è presente in ogni banda di frequenza).&lt;br /&gt;Probabilmente è di interesse solo se si sta cercando di mettere a punto il rumore del microfono. In buone condizioni, ci dovrebbe essere solo una piccola vibrazione di colore blu nella parte inferiore. Se il blu è piú di metà sul grafico, ci si trova in un ambiente molto rumoroso.</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="281"/>
        <source>Echo Analysis</source>
        <translation>Analisi dell&apos;eco</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="293"/>
        <source>Weights of the echo canceller</source>
        <translation>Peso del cancellatore di eco</translation>
    </message>
    <message>
        <location filename="../gui/AudioStats.ui" line="296"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Mostra il peso del cancellatore di eco, con il tempo crescente verso il basso e la frequenza crescente a destra.&lt;br /&gt;In teoria, dovrebbe essere nero, cioè che non esiste eco. Più comunemente, otterrai una o piú strisce orizzontali di colore bluastro che rappresentano l&apos;eco  in ritardo. Si dovrebbe poter vedere i pesi aggiornati in tempo reale.&lt;br /&gt;Notare che finché l&apos;eco è nullo, qui non sarà possibile visualizzare dati molto utili. Riproduci un po&apos; di musica e le cose dovrebbero stabilizzarsi.&lt;br /&gt;Puoi scegliere di visualizzare la parte reale o immaginaria dei pesi nel dominio della frequenza, o alternativamente il modulo e la fase calcolati. Il piú utile di questi sarà probabilmente il modulo, che è l&apos;ampiezza dell&apos;eco, e mostra quanta parte del segnale in uscita è stata rimossa in quel momento. Le altre modalità di visualizzazione sono per lo piú utili per le persone che vogliono affinare gli algoritmi per la cancellazione dell&apos;eco.&lt;br /&gt;Notare: Se l&apos;intera immagine oscilla molto mentre si è in modalità modulo, il cancellatore di echo non sta fallendo nel trovare una correlazione tra le due sorgenti di ingresso (altoparlanti e microfono). O hai un ritardo di eco veramente lungo, o una delle sorgenti di ingresso è configurata male.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Procedura guidata di sintonia dell&apos;audio</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="18"/>
        <source>Introduction</source>
        <translation>Introduzione</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="21"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Benvenuto nella procedura guidata per l&apos;impostazioni audio di RetroShare</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="32"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Questo è il wizard di sintonia dell&apos;audio per RetroShare, ti aiuterà ad impostare correttamente i livelli di input della tua scheda audio e imposterà anche i parametri corretti per il processo dei suoni su RetroShare.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="56"/>
        <source>Volume tuning</source>
        <translation>Sintonia volume</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="59"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Sintonizza il volume dell&apos;hardware del microfono sulle impostazioni ottimali.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="70"/>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there's an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it's checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation>&lt;p&gt;Apre il pannello di controllo del suono e va alle impostazioni di registrazione. Assicurati che il microfono sia selezionato come input attivo con il massimo volume di registrazione. Se c&apos;è un&apos;opzione da abilitare chiamata &quot;Preamplificazione microfono&quot; assicurati che sia selezionata.
&lt;/p&gt;
&lt;p&gt;Parla ad alta voce, come quando sei infastidito o eccitato. Poi diminuisci il volume del suono dal pannello di controllo fino a che la barra sotto rimane il più alto possibile nella zona a strisce e vuota mentre stai parlando, ma &lt;b&gt;non&lt;/b&gt; in quella crocettata.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="86"/>
        <source>Talk normally, and adjust the slider below so that the bar moves into green when you talk, and doesn&apos;t go into the orange zone.</source>
        <translation>Parla normalmente, e adatta il cursore sottostante in modo che la barra evolva nel verde mentre parli, e non vada nella zona arancio.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="130"/>
        <source>Stop looping echo for this wizard</source>
        <translation>Arresta il ciclo di eco per questo autoconfiguratore</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="150"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Applica alcune ottimizzazioni per ottenere un contrasto elevato (utile per gli utenti con problemi visivi)</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="153"/>
        <source>Use high contrast graphics</source>
        <translation>Utilizza grafica con alto contrasto</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="163"/>
        <source>Voice Activity Detection</source>
        <translation>Rilevamento dell&apos;attività della voce</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="166"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>Lascia capire a RetroShare quando stai parlando e quando stai in silenzio.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="172"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Ciò aiuterà RetroSharea capire quando si sta parlando. Il primo passo è la scelta dei dati di valore da usare.</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="184"/>
        <source>Push To Talk:</source>
        <translation>Premi Per Parlare:</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="213"/>
        <source>Voice Detection</source>
        <translation>Rilevamento voce</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="226"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>Ora devi aggiustare i seguenti 2 cursori. Le prime parole pronunciate dovrebbero finire nella zona verde (inizio del discorso). Mentre stai parlando dovresti stare dentro il giallo (discorso) e quando non stai parlando, tutto dovrebbe stare nella zona rossa (sottofondo).</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="290"/>
        <source>Continuous transmission</source>
        <translation>Trasmissione continua</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="298"/>
        <source>Finished</source>
        <translation>Finito</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="301"/>
        <source>Enjoy using RetroShare</source>
        <translation>Divertiti usando RetroShare</translation>
    </message>
    <message>
        <location filename="../gui/AudioWizard.ui" line="312"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Congratulazioni. Ora dovresti essere pronto a provare una ricca esperienza audio con RetroShare.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="93"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt; * Contributori: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="94"/>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation>&lt;br/&gt;Il plugin VOIP aggiunge VOIP alla finestra di  conversazione privata di RetroShare. Per usarlo, procedi come segue:&lt;UL&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="95"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt; regola i livelli del microfono usando il pannello di configurazione&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="96"/>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation>&lt;li&gt; verifica il tuo microfono osservando il VU-meter&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="97"/>
        <source>&lt;li&gt; in the private chat, enable sound input/output by clicking on the two VOIP icons&lt;/li&gt;&lt;/ul&gt;</source>
        <translation>&lt;li&gt; in conversazione privata, abilita l&apos;ingresso/uscita audio cliccando sulle due icone VOIP &lt;/ li&gt;&lt;/ ul&gt;</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="98"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>Il tuo amico ha bisogno di eseguire il plugin per parlare/ascoltare, ovviamente.</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="99"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;Questa è una funzione sperimentale. Non esitate a inviare commenti e suggerimenti al team sviluppatori di RS.</translation>
    </message>
    <message>
        <location filename="../VOIPPlugin.cpp" line="117"/>
        <source>RTT Statistics</source>
        <translation>RTT Statistiche</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="145"/>
        <location filename="../gui/VoipStatistics.cpp" line="147"/>
        <location filename="../gui/VoipStatistics.cpp" line="149"/>
        <source>secs</source>
        <translation>secondi</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="151"/>
        <source>Old</source>
        <translation>vecchio</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="152"/>
        <source>Now</source>
        <translation>Ora</translation>
    </message>
    <message>
        <location filename="../gui/VoipStatistics.cpp" line="361"/>
        <source>Round Trip Time:</source>
        <translation>Tempo di andata-ritorno:</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="153"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>Questo plugin offre la comunicazione vocale tra amici in RetroShare.</translation>
    </message>
</context>
<context>
    <name>VOIPPlugin</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="158"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>VoipStatistics</name>
    <message>
        <location filename="../gui/VoipStatistics.ui" line="14"/>
        <source>VoipTest Statistics</source>
        <translation>VoipTest Statistiche</translation>
    </message>
</context>
</TS>