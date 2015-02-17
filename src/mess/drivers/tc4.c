// license:BSD-3-Clause
// copyright-holders:hap
/***************************************************************************

  Coleco Total Control 4
  * TMS1400NLL MP7334-N2 (die labeled MP7334)



  TODO:
  - MCU clock is unknown

***************************************************************************/

#include "emu.h"
#include "cpu/tms0980/tms0980.h"
#include "sound/speaker.h"

#include "tc4.lh"

// The master clock is a single stage RC oscillator: R=27.3K, C=100pf,
//..
#define MASTER_CLOCK (475000)


class tc4_state : public driver_device
{
public:
	tc4_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
//		m_button_matrix(*this, "IN"),
		m_speaker(*this, "speaker")
	{ }

	required_device<cpu_device> m_maincpu;
//	required_ioport_array<2> m_button_matrix;
	required_device<speaker_sound_device> m_speaker;

	UINT16 m_r;
	UINT16 m_o;

	UINT16 m_display_state[0x10];
	UINT16 m_display_cache[0x10];
	UINT8 m_display_decay[0x100];

	DECLARE_READ8_MEMBER(read_k);
	DECLARE_WRITE16_MEMBER(write_o);
	DECLARE_WRITE16_MEMBER(write_r);

	TIMER_DEVICE_CALLBACK_MEMBER(display_decay_tick);
	void display_update();

	virtual void machine_start();
};



/***************************************************************************

  LED Display

***************************************************************************/

// The device strobes the outputs very fast, it is unnoticeable to the user.
// To prevent flickering here, we need to simulate a decay.

// decay time, in steps of 1ms
#define DISPLAY_DECAY_TIME 40


void tc4_state::display_update()
{
	UINT16 active_state[0x10];

	for (int i = 0; i < 0x10; i++)
	{
		// update current state (note: R6 as extra column!)
		m_display_state[i] = (m_r >> i & 1) ? (m_o | (m_r << 2 & 0x100)) : 0;

		active_state[i] = 0;

		for (int j = 0; j < 0x10; j++)
		{
			int di = j << 4 | i;

			// turn on powered segments
			if (m_display_state[i] >> j & 1)
				m_display_decay[di] = DISPLAY_DECAY_TIME;

			// determine active state
			int ds = (m_display_decay[di] != 0) ? 1 : 0;
			active_state[i] |= (ds << j);
		}
	}

	// on difference, send to output
	for (int i = 0; i < 0x10; i++)
		if (m_display_cache[i] != active_state[i])
		{
//			if (index_is_7segled(i))
//				output_set_digit_value(i, BITSWAP8(active_state[i],7,0,1,2,3,4,5,6) & 0x7f);

			for (int j = 0; j < 9; j++)
				output_set_lamp_value(i*10 + j, active_state[i] >> j & 1);
		}

	memcpy(m_display_cache, active_state, sizeof(m_display_cache));
}

TIMER_DEVICE_CALLBACK_MEMBER(tc4_state::display_decay_tick)
{
	// slowly turn off unpowered segments
	for (int i = 0; i < 0x100; i++)
		if (!(m_display_state[i & 0xf] >> (i>>4) & 1) && m_display_decay[i])
			m_display_decay[i]--;

	display_update();
}



/***************************************************************************

  I/O

***************************************************************************/

READ8_MEMBER(tc4_state::read_k)
{
	UINT8 k = 0;

	return k;
}

WRITE16_MEMBER(tc4_state::write_o)
{
}

WRITE16_MEMBER(tc4_state::write_r)
{
}




/***************************************************************************

  Inputs

***************************************************************************/

static INPUT_PORTS_START( tc4 )
INPUT_PORTS_END




/***************************************************************************

  Machine Config

***************************************************************************/

void tc4_state::machine_start()
{
	// zerofill
	memset(m_display_state, 0, sizeof(m_display_state));
	memset(m_display_cache, 0, sizeof(m_display_cache));
	memset(m_display_decay, 0, sizeof(m_display_decay));

	m_r = 0;
	m_o = 0;

	// register for savestates
	save_item(NAME(m_display_state));
	save_item(NAME(m_display_cache));
	save_item(NAME(m_display_decay));

	save_item(NAME(m_r));
	save_item(NAME(m_o));
}


static MACHINE_CONFIG_START( tc4, tc4_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", TMS1400, MASTER_CLOCK)
	MCFG_TMS1XXX_READ_K_CB(READ8(tc4_state, read_k))
	MCFG_TMS1XXX_WRITE_O_CB(WRITE16(tc4_state, write_o))
	MCFG_TMS1XXX_WRITE_R_CB(WRITE16(tc4_state, write_r))

	MCFG_TIMER_DRIVER_ADD_PERIODIC("display_decay", tc4_state, display_decay_tick, attotime::from_msec(1))

	MCFG_DEFAULT_LAYOUT(layout_tc4)

	/* no video! */

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tc4 )
	ROM_REGION( 0x1000, "maincpu", 0 )
	ROM_LOAD( "tms1400nll_mp7334", 0x0000, 0x1000, CRC(923f3821) SHA1(a9ae342d7ff8dae1dedcd1e4984bcfae68586581) )

	ROM_REGION( 867, "maincpu:mpla", 0 )
	ROM_LOAD( "tms1100_default_mpla.pla", 0, 867, CRC(62445fc9) SHA1(d6297f2a4bc7a870b76cc498d19dbb0ce7d69fec) )
	ROM_REGION( 557, "maincpu:opla", 0 )
	ROM_LOAD( "tms1400_tc4_opla.pla", 0, 557, CRC(3b908725) SHA1(f83bf5faa5b3cb51f87adc1639b00d6f9a71ad19) )
ROM_END



CONS( 1981, tc4, 0, 0, tc4, tc4, driver_device, 0, "Coleco", "Total Control 4", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
