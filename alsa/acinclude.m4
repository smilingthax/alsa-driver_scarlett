dnl ALSA soundcard-configuration
dnl Find out which cards to compile driver for
dnl Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>

AC_DEFUN(ALSA_CARDS_INIT, [
	CONFIG_PERSIST="0"
	CONFIG_SND="0"
	CONFIG_SND_MIXER="0"
	CONFIG_SND_PCM="0"
	CONFIG_SND_MIDI="0"
	CONFIG_SND_TIMER="0"
	CONFIG_SND_HWDEP="0"
	CONFIG_SND_PCM1="0"
	CONFIG_SND_SEQ="0"
	CONFIG_SND_SEQ_DEVICE="0"
	CONFIG_SND_SEQ_MIDI="0"
	CONFIG_SND_SEQ_MIDI_EMUL="0"
	CONFIG_SND_SEQ_INSTR="0"
	CONFIG_SND_AINSTR_SIMPLE="0"
	CONFIG_SND_AINSTR_GF1="0"
	CONFIG_SND_AINSTR_IW="0"
	CONFIG_SND_DETECT="0"
	CONFIG_SND_MPU401_UART="0"
	CONFIG_SND_OPL3="0"
	CONFIG_SND_AC97_CODEC="0"
	CONFIG_SND_AK4531_CODEC="0"
	CONFIG_SND_UART16550="0"
	CONFIG_SND_GUS="0"
	CONFIG_SND_SYNTH_GUS="0"
	CONFIG_SND_I2C="0"
	CONFIG_SND_TEA6330T="0"
	CONFIG_SND_AD1848="0"
	CONFIG_SND_CS4231="0"
	CONFIG_SND_ES1688="0"
	CONFIG_SND_ES18XX="0"
	CONFIG_SND_CS4236="0"
	CONFIG_SND_WAVEFRONT_SYNTH="0"
	CONFIG_SND_WAVEFRONT_FX="0"
	CONFIG_SND_S3_86C617="0"
	CONFIG_SND_ENS1370="0"
	CONFIG_SND_ENS1371="0"
	CONFIG_SND_ES1938="0"
	CONFIG_SND_TRIDENT_DX_NX="0"
	CONFIG_SND_CS461X="0"
	CONFIG_SND_FM801="0"
	CONFIG_SND_SB8_DSP="0"
	CONFIG_SND_SB16_DSP="0"
	CONFIG_SND_SB16_CSP="0"
	CONFIG_SND_SYNTH_EMU8000="0"
	CONFIG_SND_HAL2="0"
	CONFIG_SND_CARD_DUMMY="0"
	CONFIG_SND_CARD_INTERWAVE="0"
	CONFIG_SND_CARD_INTERWAVE_STB="0"
	CONFIG_SND_CARD_GUSMAX="0"
	CONFIG_SND_CARD_GUSEXTREME="0"
	CONFIG_SND_CARD_GUSCLASSIC="0"
	CONFIG_SND_CARD_ES1688="0"
	CONFIG_SND_CARD_ES18XX="0"
	CONFIG_SND_CARD_SB8="0"
	CONFIG_SND_CARD_SB16="0"
	CONFIG_SND_CARD_SBAWE="0"
	CONFIG_SND_CARD_OPL3SA2="0"
	CONFIG_SND_CARD_MOZART="0"
	CONFIG_SND_CARD_SONICVIBES="0"
	CONFIG_SND_CARD_ENS1370="0"
	CONFIG_SND_CARD_ENS1371="0"
	CONFIG_SND_CARD_AD1848="0"
	CONFIG_SND_CARD_ALS100="0"
	CONFIG_SND_CARD_AZT2320="0"
	CONFIG_SND_CARD_CS4231="0"
	CONFIG_SND_CARD_CS4232="0"
	CONFIG_SND_CARD_CS4236="0"
	CONFIG_SND_CARD_CS461X="0"
	CONFIG_SND_CARD_ES968="0"
	CONFIG_SND_CARD_DT0197H="0"
	CONFIG_SND_CARD_FM801="0"
	CONFIG_SND_CARD_ES1938="0"
	CONFIG_SND_CARD_OPTI9XX="0"
	CONFIG_SND_CARD_SERIAL="0"
	CONFIG_SND_CARD_TRID4DWAVE="0"
	CONFIG_SND_CARD_SGALAXY="0"
	CONFIG_SND_CARD_WAVEFRONT="0"
	CONFIG_SND_CARD_HAL2="0"
	CONFIG_SND_CARD_CMI8330="0"
])

AC_DEFUN(ALSA_CARDS_ALL, [
	CONFIG_PERSIST="1"
	AC_DEFINE(CONFIG_PERSIST)
	CONFIG_SND="1"
	AC_DEFINE(CONFIG_SND)
	CONFIG_SND_MIXER="1"
	AC_DEFINE(CONFIG_SND_MIXER)
	CONFIG_SND_PCM="1"
	AC_DEFINE(CONFIG_SND_PCM)
	CONFIG_SND_MIDI="1"
	AC_DEFINE(CONFIG_SND_MIDI)
	CONFIG_SND_TIMER="1"
	AC_DEFINE(CONFIG_SND_TIMER)
	CONFIG_SND_HWDEP="1"
	AC_DEFINE(CONFIG_SND_HWDEP)
	CONFIG_SND_PCM1="1"
	AC_DEFINE(CONFIG_SND_PCM1)
	CONFIG_SND_SEQ="1"
	AC_DEFINE(CONFIG_SND_SEQ)
	CONFIG_SND_SEQ_DEVICE="1"
	AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
	CONFIG_SND_SEQ_MIDI="1"
	AC_DEFINE(CONFIG_SND_SEQ_MIDI)
	CONFIG_SND_SEQ_MIDI_EMUL="1"
	AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
	CONFIG_SND_SEQ_INSTR="1"
	AC_DEFINE(CONFIG_SND_SEQ_INSTR)
	CONFIG_SND_AINSTR_SIMPLE="1"
	AC_DEFINE(CONFIG_SND_AINSTR_SIMPLE)
	CONFIG_SND_AINSTR_GF1="1"
	AC_DEFINE(CONFIG_SND_AINSTR_GF1)
	CONFIG_SND_AINSTR_IW="1"
	AC_DEFINE(CONFIG_SND_AINSTR_IW)
	CONFIG_SND_DETECT="1"
	AC_DEFINE(CONFIG_SND_DETECT)
	CONFIG_SND_MPU401_UART="1"
	AC_DEFINE(CONFIG_SND_MPU401_UART)
	CONFIG_SND_OPL3="1"
	AC_DEFINE(CONFIG_SND_OPL3)
	CONFIG_SND_AC97_CODEC="1"
	AC_DEFINE(CONFIG_SND_AC97_CODEC)
	CONFIG_SND_AK4531_CODEC="1"
	AC_DEFINE(CONFIG_SND_AK4531_CODEC)
	CONFIG_SND_UART16550="1"
	AC_DEFINE(CONFIG_SND_UART16550)
	CONFIG_SND_GUS="1"
	AC_DEFINE(CONFIG_SND_GUS)
	CONFIG_SND_SYNTH_GUS="1"
	AC_DEFINE(CONFIG_SND_SYNTH_GUS)
	CONFIG_SND_I2C="1"
	AC_DEFINE(CONFIG_SND_I2C)
	CONFIG_SND_TEA6330T="1"
	AC_DEFINE(CONFIG_SND_TEA6330T)
	CONFIG_SND_AD1848="1"
	AC_DEFINE(CONFIG_SND_AD1848)
	CONFIG_SND_CS4231="1"
	AC_DEFINE(CONFIG_SND_CS4231)
	CONFIG_SND_ES1688="1"
	AC_DEFINE(CONFIG_SND_ES1688)
	CONFIG_SND_ES18XX="1"
	AC_DEFINE(CONFIG_SND_ES18XX)
	CONFIG_SND_CS4236="1"
	AC_DEFINE(CONFIG_SND_CS4236)
	CONFIG_SND_WAVEFRONT_SYNTH="1"
	AC_DEFINE(CONFIG_SND_WAVEFRONT_SYNTH)
	CONFIG_SND_WAVEFRONT_FX="1"
	AC_DEFINE(CONFIG_SND_WAVEFRONT_FX)
	CONFIG_SND_S3_86C617="1"
	AC_DEFINE(CONFIG_SND_S3_86C617)
	CONFIG_SND_ENS1370="1"
	AC_DEFINE(CONFIG_SND_ENS1370)
	CONFIG_SND_ENS1371="1"
	AC_DEFINE(CONFIG_SND_ENS1371)
	CONFIG_SND_ES1938="1"
	AC_DEFINE(CONFIG_SND_ES1938)
	CONFIG_SND_TRIDENT_DX_NX="1"
	AC_DEFINE(CONFIG_SND_TRIDENT_DX_NX)
	CONFIG_SND_CS461X="1"
	AC_DEFINE(CONFIG_SND_CS461X)
	CONFIG_SND_FM801="1"
	AC_DEFINE(CONFIG_SND_FM801)
	CONFIG_SND_SB8_DSP="1"
	AC_DEFINE(CONFIG_SND_SB8_DSP)
	CONFIG_SND_SB16_DSP="1"
	AC_DEFINE(CONFIG_SND_SB16_DSP)
	CONFIG_SND_SB16_CSP="1"
	AC_DEFINE(CONFIG_SND_SB16_CSP)
	CONFIG_SND_SYNTH_EMU8000="1"
	AC_DEFINE(CONFIG_SND_SYNTH_EMU8000)
	CONFIG_SND_HAL2="1"
	AC_DEFINE(CONFIG_SND_HAL2)
	CONFIG_SND_CARD_DUMMY="1"
	AC_DEFINE(CONFIG_SND_CARD_DUMMY)
	CONFIG_SND_CARD_INTERWAVE="1"
	AC_DEFINE(CONFIG_SND_CARD_INTERWAVE)
	CONFIG_SND_CARD_INTERWAVE_STB="1"
	AC_DEFINE(CONFIG_SND_CARD_INTERWAVE_STB)
	CONFIG_SND_CARD_GUSMAX="1"
	AC_DEFINE(CONFIG_SND_CARD_GUSMAX)
	CONFIG_SND_CARD_GUSEXTREME="1"
	AC_DEFINE(CONFIG_SND_CARD_GUSEXTREME)
	CONFIG_SND_CARD_GUSCLASSIC="1"
	AC_DEFINE(CONFIG_SND_CARD_GUSCLASSIC)
	CONFIG_SND_CARD_ES1688="1"
	AC_DEFINE(CONFIG_SND_CARD_ES1688)
	CONFIG_SND_CARD_ES18XX="1"
	AC_DEFINE(CONFIG_SND_CARD_ES18XX)
	CONFIG_SND_CARD_SB8="1"
	AC_DEFINE(CONFIG_SND_CARD_SB8)
	CONFIG_SND_CARD_SB16="1"
	AC_DEFINE(CONFIG_SND_CARD_SB16)
	CONFIG_SND_CARD_SBAWE="1"
	AC_DEFINE(CONFIG_SND_CARD_SBAWE)
	CONFIG_SND_CARD_OPL3SA2="1"
	AC_DEFINE(CONFIG_SND_CARD_OPL3SA2)
	CONFIG_SND_CARD_MOZART="1"
	AC_DEFINE(CONFIG_SND_CARD_MOZART)
	CONFIG_SND_CARD_SONICVIBES="1"
	AC_DEFINE(CONFIG_SND_CARD_SONICVIBES)
	CONFIG_SND_CARD_ENS1370="1"
	AC_DEFINE(CONFIG_SND_CARD_ENS1370)
	CONFIG_SND_CARD_ENS1371="1"
	AC_DEFINE(CONFIG_SND_CARD_ENS1371)
	CONFIG_SND_CARD_AD1848="1"
	AC_DEFINE(CONFIG_SND_CARD_AD1848)
	CONFIG_SND_CARD_ALS100="1"
	AC_DEFINE(CONFIG_SND_CARD_ALS100)
	CONFIG_SND_CARD_AZT2320="1"
	AC_DEFINE(CONFIG_SND_CARD_AZT2320)
	CONFIG_SND_CARD_CS4231="1"
	AC_DEFINE(CONFIG_SND_CARD_CS4231)
	CONFIG_SND_CARD_CS4232="1"
	AC_DEFINE(CONFIG_SND_CARD_CS4232)
	CONFIG_SND_CARD_CS4236="1"
	AC_DEFINE(CONFIG_SND_CARD_CS4236)
	CONFIG_SND_CARD_CS461X="1"
	AC_DEFINE(CONFIG_SND_CARD_CS461X)
	CONFIG_SND_CARD_ES968="1"
	AC_DEFINE(CONFIG_SND_CARD_ES968)
	CONFIG_SND_CARD_DT0197H="1"
	AC_DEFINE(CONFIG_SND_CARD_DT0197H)
	CONFIG_SND_CARD_FM801="1"
	AC_DEFINE(CONFIG_SND_CARD_FM801)
	CONFIG_SND_CARD_ES1938="1"
	AC_DEFINE(CONFIG_SND_CARD_ES1938)
	CONFIG_SND_CARD_OPTI9XX="1"
	AC_DEFINE(CONFIG_SND_CARD_OPTI9XX)
	CONFIG_SND_CARD_SERIAL="1"
	AC_DEFINE(CONFIG_SND_CARD_SERIAL)
	CONFIG_SND_CARD_TRID4DWAVE="1"
	AC_DEFINE(CONFIG_SND_CARD_TRID4DWAVE)
	CONFIG_SND_CARD_SGALAXY="1"
	AC_DEFINE(CONFIG_SND_CARD_SGALAXY)
	CONFIG_SND_CARD_WAVEFRONT="1"
	AC_DEFINE(CONFIG_SND_CARD_WAVEFRONT)
	CONFIG_SND_CARD_HAL2="1"
	AC_DEFINE(CONFIG_SND_CARD_HAL2)
	CONFIG_SND_CARD_CMI8330="1"
	AC_DEFINE(CONFIG_SND_CARD_CMI8330)
])

AC_DEFUN(ALSA_CARDS_SELECT, [
dnl Check for which cards to compile driver for...
AC_MSG_CHECKING(for which soundcards to compile driver for)
AC_ARG_WITH(cards,
  [  --with-cards=<list>     compile driver for cards in <list>. ]
  [                        cards may be separated with commas. ]
  [                        "all" compiles all drivers ],
  cards="$withval", cards="all")
if test "$cards" = "all"; then
  ALSA_CARDS_ALL
  AC_MSG_RESULT(all)
else
  cards=`echo $cards | sed 's/,/ /g'`
  for card in $cards
  do
    case "$card" in
	card-dummy)
		CONFIG_SND_CARD_DUMMY="1"
		AC_DEFINE(CONFIG_SND_CARD_DUMMY)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		;;
	card-interwave)
		CONFIG_SND_CARD_INTERWAVE="1"
		AC_DEFINE(CONFIG_SND_CARD_INTERWAVE)
		CONFIG_SND_GUS="1"
		AC_DEFINE(CONFIG_SND_GUS)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_SYNTH_GUS="1"
		AC_DEFINE(CONFIG_SND_SYNTH_GUS)
		CONFIG_SND_SEQ_MIDI_EMUL="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
		CONFIG_SND_AINSTR_IW="1"
		AC_DEFINE(CONFIG_SND_AINSTR_IW)
		CONFIG_SND_SEQ_INSTR="1"
		AC_DEFINE(CONFIG_SND_SEQ_INSTR)
		CONFIG_SND_AINSTR_GF1="1"
		AC_DEFINE(CONFIG_SND_AINSTR_GF1)
		CONFIG_SND_AINSTR_SIMPLE="1"
		AC_DEFINE(CONFIG_SND_AINSTR_SIMPLE)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		;;
	card-interwave-stb)
		CONFIG_SND_CARD_INTERWAVE_STB="1"
		AC_DEFINE(CONFIG_SND_CARD_INTERWAVE_STB)
		CONFIG_SND_GUS="1"
		AC_DEFINE(CONFIG_SND_GUS)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_SYNTH_GUS="1"
		AC_DEFINE(CONFIG_SND_SYNTH_GUS)
		CONFIG_SND_SEQ_MIDI_EMUL="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
		CONFIG_SND_AINSTR_IW="1"
		AC_DEFINE(CONFIG_SND_AINSTR_IW)
		CONFIG_SND_SEQ_INSTR="1"
		AC_DEFINE(CONFIG_SND_SEQ_INSTR)
		CONFIG_SND_AINSTR_GF1="1"
		AC_DEFINE(CONFIG_SND_AINSTR_GF1)
		CONFIG_SND_AINSTR_SIMPLE="1"
		AC_DEFINE(CONFIG_SND_AINSTR_SIMPLE)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_TEA6330T="1"
		AC_DEFINE(CONFIG_SND_TEA6330T)
		CONFIG_SND_I2C="1"
		AC_DEFINE(CONFIG_SND_I2C)
		;;
	card-gusmax)
		CONFIG_SND_CARD_GUSMAX="1"
		AC_DEFINE(CONFIG_SND_CARD_GUSMAX)
		CONFIG_SND_GUS="1"
		AC_DEFINE(CONFIG_SND_GUS)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_SYNTH_GUS="1"
		AC_DEFINE(CONFIG_SND_SYNTH_GUS)
		CONFIG_SND_SEQ_MIDI_EMUL="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
		CONFIG_SND_AINSTR_IW="1"
		AC_DEFINE(CONFIG_SND_AINSTR_IW)
		CONFIG_SND_SEQ_INSTR="1"
		AC_DEFINE(CONFIG_SND_SEQ_INSTR)
		CONFIG_SND_AINSTR_GF1="1"
		AC_DEFINE(CONFIG_SND_AINSTR_GF1)
		CONFIG_SND_AINSTR_SIMPLE="1"
		AC_DEFINE(CONFIG_SND_AINSTR_SIMPLE)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		;;
	card-gusextreme)
		CONFIG_SND_CARD_GUSEXTREME="1"
		AC_DEFINE(CONFIG_SND_CARD_GUSEXTREME)
		CONFIG_SND_GUS="1"
		AC_DEFINE(CONFIG_SND_GUS)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_SYNTH_GUS="1"
		AC_DEFINE(CONFIG_SND_SYNTH_GUS)
		CONFIG_SND_SEQ_MIDI_EMUL="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
		CONFIG_SND_AINSTR_IW="1"
		AC_DEFINE(CONFIG_SND_AINSTR_IW)
		CONFIG_SND_SEQ_INSTR="1"
		AC_DEFINE(CONFIG_SND_SEQ_INSTR)
		CONFIG_SND_AINSTR_GF1="1"
		AC_DEFINE(CONFIG_SND_AINSTR_GF1)
		CONFIG_SND_AINSTR_SIMPLE="1"
		AC_DEFINE(CONFIG_SND_AINSTR_SIMPLE)
		CONFIG_SND_ES1688="1"
		AC_DEFINE(CONFIG_SND_ES1688)
		;;
	card-gusclassic)
		CONFIG_SND_CARD_GUSCLASSIC="1"
		AC_DEFINE(CONFIG_SND_CARD_GUSCLASSIC)
		CONFIG_SND_GUS="1"
		AC_DEFINE(CONFIG_SND_GUS)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_SYNTH_GUS="1"
		AC_DEFINE(CONFIG_SND_SYNTH_GUS)
		CONFIG_SND_SEQ_MIDI_EMUL="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
		CONFIG_SND_AINSTR_IW="1"
		AC_DEFINE(CONFIG_SND_AINSTR_IW)
		CONFIG_SND_SEQ_INSTR="1"
		AC_DEFINE(CONFIG_SND_SEQ_INSTR)
		CONFIG_SND_AINSTR_GF1="1"
		AC_DEFINE(CONFIG_SND_AINSTR_GF1)
		CONFIG_SND_AINSTR_SIMPLE="1"
		AC_DEFINE(CONFIG_SND_AINSTR_SIMPLE)
		;;
	card-es1688)
		CONFIG_SND_CARD_ES1688="1"
		AC_DEFINE(CONFIG_SND_CARD_ES1688)
		CONFIG_SND_ES1688="1"
		AC_DEFINE(CONFIG_SND_ES1688)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-es18xx)
		CONFIG_SND_CARD_ES18XX="1"
		AC_DEFINE(CONFIG_SND_CARD_ES18XX)
		CONFIG_SND_ES18XX="1"
		AC_DEFINE(CONFIG_SND_ES18XX)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-sb8)
		CONFIG_SND_CARD_SB8="1"
		AC_DEFINE(CONFIG_SND_CARD_SB8)
		CONFIG_SND_SB8_DSP="1"
		AC_DEFINE(CONFIG_SND_SB8_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-sb16)
		CONFIG_SND_CARD_SB16="1"
		AC_DEFINE(CONFIG_SND_CARD_SB16)
		CONFIG_SND_SB16_DSP="1"
		AC_DEFINE(CONFIG_SND_SB16_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_SB16_CSP="1"
		AC_DEFINE(CONFIG_SND_SB16_CSP)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		;;
	card-sbawe)
		CONFIG_SND_CARD_SBAWE="1"
		AC_DEFINE(CONFIG_SND_CARD_SBAWE)
		CONFIG_SND_SB16_DSP="1"
		AC_DEFINE(CONFIG_SND_SB16_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_SB16_CSP="1"
		AC_DEFINE(CONFIG_SND_SB16_CSP)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_SYNTH_EMU8000="1"
		AC_DEFINE(CONFIG_SND_SYNTH_EMU8000)
		CONFIG_SND_SEQ_MIDI_EMUL="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI_EMUL)
		;;
	card-opl3sa2)
		CONFIG_SND_CARD_OPL3SA2="1"
		AC_DEFINE(CONFIG_SND_CARD_OPL3SA2)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-mozart)
		CONFIG_SND_CARD_MOZART="1"
		AC_DEFINE(CONFIG_SND_CARD_MOZART)
		CONFIG_SND_AD1848="1"
		AC_DEFINE(CONFIG_SND_AD1848)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		;;
	card-sonicvibes)
		CONFIG_SND_CARD_SONICVIBES="1"
		AC_DEFINE(CONFIG_SND_CARD_SONICVIBES)
		CONFIG_SND_S3_86C617="1"
		AC_DEFINE(CONFIG_SND_S3_86C617)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-ens1370)
		CONFIG_SND_CARD_ENS1370="1"
		AC_DEFINE(CONFIG_SND_CARD_ENS1370)
		CONFIG_SND_ENS1370="1"
		AC_DEFINE(CONFIG_SND_ENS1370)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_AK4531_CODEC="1"
		AC_DEFINE(CONFIG_SND_AK4531_CODEC)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		;;
	card-ens1371)
		CONFIG_SND_CARD_ENS1371="1"
		AC_DEFINE(CONFIG_SND_CARD_ENS1371)
		CONFIG_SND_ENS1371="1"
		AC_DEFINE(CONFIG_SND_ENS1371)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_AC97_CODEC="1"
		AC_DEFINE(CONFIG_SND_AC97_CODEC)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		;;
	card-ad1848)
		CONFIG_SND_CARD_AD1848="1"
		AC_DEFINE(CONFIG_SND_CARD_AD1848)
		CONFIG_SND_AD1848="1"
		AC_DEFINE(CONFIG_SND_AD1848)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		;;
	card-als100)
		CONFIG_SND_CARD_ALS100="1"
		AC_DEFINE(CONFIG_SND_CARD_ALS100)
		CONFIG_SND_SB16_DSP="1"
		AC_DEFINE(CONFIG_SND_SB16_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-azt2320)
		CONFIG_SND_CARD_AZT2320="1"
		AC_DEFINE(CONFIG_SND_CARD_AZT2320)
		CONFIG_SND_SB8_DSP="1"
		AC_DEFINE(CONFIG_SND_SB8_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-cs4231)
		CONFIG_SND_CARD_CS4231="1"
		AC_DEFINE(CONFIG_SND_CARD_CS4231)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		;;
	card-cs4232)
		CONFIG_SND_CARD_CS4232="1"
		AC_DEFINE(CONFIG_SND_CARD_CS4232)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-cs4236)
		CONFIG_SND_CARD_CS4236="1"
		AC_DEFINE(CONFIG_SND_CARD_CS4236)
		CONFIG_SND_CS4236="1"
		AC_DEFINE(CONFIG_SND_CS4236)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-cs461x)
		CONFIG_SND_CARD_CS461X="1"
		AC_DEFINE(CONFIG_SND_CARD_CS461X)
		CONFIG_SND_CS461X="1"
		AC_DEFINE(CONFIG_SND_CS461X)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_AC97_CODEC="1"
		AC_DEFINE(CONFIG_SND_AC97_CODEC)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		;;
	card-es968)
		CONFIG_SND_CARD_ES968="1"
		AC_DEFINE(CONFIG_SND_CARD_ES968)
		CONFIG_SND_SB8_DSP="1"
		AC_DEFINE(CONFIG_SND_SB8_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		;;
	card-dt0197h)
		CONFIG_SND_CARD_DT0197H="1"
		AC_DEFINE(CONFIG_SND_CARD_DT0197H)
		CONFIG_SND_SB16_DSP="1"
		AC_DEFINE(CONFIG_SND_SB16_DSP)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-fm801)
		CONFIG_SND_CARD_FM801="1"
		AC_DEFINE(CONFIG_SND_CARD_FM801)
		CONFIG_SND_FM801="1"
		AC_DEFINE(CONFIG_SND_FM801)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_AC97_CODEC="1"
		AC_DEFINE(CONFIG_SND_AC97_CODEC)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-es1938)
		CONFIG_SND_CARD_ES1938="1"
		AC_DEFINE(CONFIG_SND_CARD_ES1938)
		CONFIG_SND_ES1938="1"
		AC_DEFINE(CONFIG_SND_ES1938)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-opti9xx)
		CONFIG_SND_CARD_OPTI9XX="1"
		AC_DEFINE(CONFIG_SND_CARD_OPTI9XX)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_AD1848="1"
		AC_DEFINE(CONFIG_SND_AD1848)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		;;
	card-serial)
		CONFIG_SND_CARD_SERIAL="1"
		AC_DEFINE(CONFIG_SND_CARD_SERIAL)
		CONFIG_SND_UART16550="1"
		AC_DEFINE(CONFIG_SND_UART16550)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		;;
	card-trid4dwave)
		CONFIG_SND_CARD_TRID4DWAVE="1"
		AC_DEFINE(CONFIG_SND_CARD_TRID4DWAVE)
		CONFIG_SND_TRIDENT_DX_NX="1"
		AC_DEFINE(CONFIG_SND_TRIDENT_DX_NX)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_AC97_CODEC="1"
		AC_DEFINE(CONFIG_SND_AC97_CODEC)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		;;
	card-sgalaxy)
		CONFIG_SND_CARD_SGALAXY="1"
		AC_DEFINE(CONFIG_SND_CARD_SGALAXY)
		CONFIG_SND_AD1848="1"
		AC_DEFINE(CONFIG_SND_AD1848)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		;;
	card-wavefront)
		CONFIG_SND_CARD_WAVEFRONT="1"
		AC_DEFINE(CONFIG_SND_CARD_WAVEFRONT)
		CONFIG_SND_CS4231="1"
		AC_DEFINE(CONFIG_SND_CS4231)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_MPU401_UART="1"
		AC_DEFINE(CONFIG_SND_MPU401_UART)
		CONFIG_SND_MIDI="1"
		AC_DEFINE(CONFIG_SND_MIDI)
		CONFIG_SND_SEQ_MIDI="1"
		AC_DEFINE(CONFIG_SND_SEQ_MIDI)
		CONFIG_SND_SEQ="1"
		AC_DEFINE(CONFIG_SND_SEQ)
		CONFIG_SND_SEQ_DEVICE="1"
		AC_DEFINE(CONFIG_SND_SEQ_DEVICE)
		CONFIG_SND_OPL3="1"
		AC_DEFINE(CONFIG_SND_OPL3)
		CONFIG_SND_HWDEP="1"
		AC_DEFINE(CONFIG_SND_HWDEP)
		CONFIG_SND_WAVEFRONT_SYNTH="1"
		AC_DEFINE(CONFIG_SND_WAVEFRONT_SYNTH)
		CONFIG_SND_WAVEFRONT_FX="1"
		AC_DEFINE(CONFIG_SND_WAVEFRONT_FX)
		;;
	card-hal2)
		CONFIG_SND_CARD_HAL2="1"
		AC_DEFINE(CONFIG_SND_CARD_HAL2)
		CONFIG_SND_HAL2="1"
		AC_DEFINE(CONFIG_SND_HAL2)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		;;
	card-cmi8330)
		CONFIG_SND_CARD_CMI8330="1"
		AC_DEFINE(CONFIG_SND_CARD_CMI8330)
		CONFIG_SND_AD1848="1"
		AC_DEFINE(CONFIG_SND_AD1848)
		CONFIG_SND_PCM1="1"
		AC_DEFINE(CONFIG_SND_PCM1)
		CONFIG_SND_PCM="1"
		AC_DEFINE(CONFIG_SND_PCM)
		CONFIG_SND="1"
		AC_DEFINE(CONFIG_SND)
		CONFIG_PERSIST="1"
		AC_DEFINE(CONFIG_PERSIST)
		CONFIG_SND_TIMER="1"
		AC_DEFINE(CONFIG_SND_TIMER)
		CONFIG_SND_MIXER="1"
		AC_DEFINE(CONFIG_SND_MIXER)
		CONFIG_SND_SB16_DSP="1"
		AC_DEFINE(CONFIG_SND_SB16_DSP)
		;;
	*)
		echo "Unknown soundcard $card, exiting!"
		exit 1
		;;
    esac
  done
  AC_MSG_RESULT($cards)
fi
AC_SUBST(CONFIG_PERSIST)
AC_SUBST(CONFIG_SND)
AC_SUBST(CONFIG_SND_MIXER)
AC_SUBST(CONFIG_SND_PCM)
AC_SUBST(CONFIG_SND_MIDI)
AC_SUBST(CONFIG_SND_TIMER)
AC_SUBST(CONFIG_SND_HWDEP)
AC_SUBST(CONFIG_SND_PCM1)
AC_SUBST(CONFIG_SND_SEQ)
AC_SUBST(CONFIG_SND_SEQ_DEVICE)
AC_SUBST(CONFIG_SND_SEQ_MIDI)
AC_SUBST(CONFIG_SND_SEQ_MIDI_EMUL)
AC_SUBST(CONFIG_SND_SEQ_INSTR)
AC_SUBST(CONFIG_SND_AINSTR_SIMPLE)
AC_SUBST(CONFIG_SND_AINSTR_GF1)
AC_SUBST(CONFIG_SND_AINSTR_IW)
AC_SUBST(CONFIG_SND_DETECT)
AC_SUBST(CONFIG_SND_MPU401_UART)
AC_SUBST(CONFIG_SND_OPL3)
AC_SUBST(CONFIG_SND_AC97_CODEC)
AC_SUBST(CONFIG_SND_AK4531_CODEC)
AC_SUBST(CONFIG_SND_UART16550)
AC_SUBST(CONFIG_SND_GUS)
AC_SUBST(CONFIG_SND_SYNTH_GUS)
AC_SUBST(CONFIG_SND_I2C)
AC_SUBST(CONFIG_SND_TEA6330T)
AC_SUBST(CONFIG_SND_AD1848)
AC_SUBST(CONFIG_SND_CS4231)
AC_SUBST(CONFIG_SND_ES1688)
AC_SUBST(CONFIG_SND_ES18XX)
AC_SUBST(CONFIG_SND_CS4236)
AC_SUBST(CONFIG_SND_WAVEFRONT_SYNTH)
AC_SUBST(CONFIG_SND_WAVEFRONT_FX)
AC_SUBST(CONFIG_SND_S3_86C617)
AC_SUBST(CONFIG_SND_ENS1370)
AC_SUBST(CONFIG_SND_ENS1371)
AC_SUBST(CONFIG_SND_ES1938)
AC_SUBST(CONFIG_SND_TRIDENT_DX_NX)
AC_SUBST(CONFIG_SND_CS461X)
AC_SUBST(CONFIG_SND_FM801)
AC_SUBST(CONFIG_SND_SB8_DSP)
AC_SUBST(CONFIG_SND_SB16_DSP)
AC_SUBST(CONFIG_SND_SB16_CSP)
AC_SUBST(CONFIG_SND_SYNTH_EMU8000)
AC_SUBST(CONFIG_SND_HAL2)
AC_SUBST(CONFIG_SND_CARD_DUMMY)
AC_SUBST(CONFIG_SND_CARD_INTERWAVE)
AC_SUBST(CONFIG_SND_CARD_INTERWAVE_STB)
AC_SUBST(CONFIG_SND_CARD_GUSMAX)
AC_SUBST(CONFIG_SND_CARD_GUSEXTREME)
AC_SUBST(CONFIG_SND_CARD_GUSCLASSIC)
AC_SUBST(CONFIG_SND_CARD_ES1688)
AC_SUBST(CONFIG_SND_CARD_ES18XX)
AC_SUBST(CONFIG_SND_CARD_SB8)
AC_SUBST(CONFIG_SND_CARD_SB16)
AC_SUBST(CONFIG_SND_CARD_SBAWE)
AC_SUBST(CONFIG_SND_CARD_OPL3SA2)
AC_SUBST(CONFIG_SND_CARD_MOZART)
AC_SUBST(CONFIG_SND_CARD_SONICVIBES)
AC_SUBST(CONFIG_SND_CARD_ENS1370)
AC_SUBST(CONFIG_SND_CARD_ENS1371)
AC_SUBST(CONFIG_SND_CARD_AD1848)
AC_SUBST(CONFIG_SND_CARD_ALS100)
AC_SUBST(CONFIG_SND_CARD_AZT2320)
AC_SUBST(CONFIG_SND_CARD_CS4231)
AC_SUBST(CONFIG_SND_CARD_CS4232)
AC_SUBST(CONFIG_SND_CARD_CS4236)
AC_SUBST(CONFIG_SND_CARD_CS461X)
AC_SUBST(CONFIG_SND_CARD_ES968)
AC_SUBST(CONFIG_SND_CARD_DT0197H)
AC_SUBST(CONFIG_SND_CARD_FM801)
AC_SUBST(CONFIG_SND_CARD_ES1938)
AC_SUBST(CONFIG_SND_CARD_OPTI9XX)
AC_SUBST(CONFIG_SND_CARD_SERIAL)
AC_SUBST(CONFIG_SND_CARD_TRID4DWAVE)
AC_SUBST(CONFIG_SND_CARD_SGALAXY)
AC_SUBST(CONFIG_SND_CARD_WAVEFRONT)
AC_SUBST(CONFIG_SND_CARD_HAL2)
AC_SUBST(CONFIG_SND_CARD_CMI8330)
])
