#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_mixer.h"

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* set this to any of 512,1024,2048,4096              */
/* the lower it is, the more FPS shown and CPU needed */
#define BUFFER 1024
#define W 640 /* NEVER make this be less than BUFFER! */
#define H 480
#define H2 (H/2)
#define H4 (H/4)
#define Y(sample) (((sample)*H)/4/0x7fff)

Sint16 stream[2][BUFFER*2*2];
int len=BUFFER*2*2, done=0, need_refresh=0, bits=0, which=0,
	sample_size=0, position=0, rate=0;
SDL_Surface *s=NULL;
Uint32 flips=0;
Uint32 black,white;
float dy;

/******************************************************************************/
/* some simple exit and error routines                                        */

void errorv(char *str, va_list ap)
{
	vfprintf(stderr,str,ap);
	fprintf(stderr,": %s.\n", SDL_GetError());
}

void cleanExit(char *str,...)
{
	va_list ap;
	va_start(ap, str);
	errorv(str,ap);
	va_end(ap);
	Mix_CloseAudio();
	SDL_Quit();
	exit(1);
}

/******************************************************************************/
/* the postmix processor, only copies the stream buffer and indicates         */
/* a need for a screen refresh                                                */

static void postmix(void *udata, Uint8 *_stream, int _len)
{
	position+=_len/sample_size;
	/* fprintf(stderr,"pos=%7.2f seconds \r",position/(float)rate); */
	if(need_refresh)
		return;
	/* save the stream buffer and indicate that we need a redraw */
	len=_len;
	memcpy(stream[(which+1)%2],_stream,len>s->w*4?s->w*4:len);
	which=(which+1)%2;
	need_refresh=1;
}

/******************************************************************************/
/* redraw the wav and reset the need_refresh indicator                        */

void refresh()
{
	int x;
	Sint16 *buf;

	/*fprintf(stderr,"len=%d   \r",len); */

	buf=stream[which];
	need_refresh=0;
	
	SDL_LockSurface(s);
	/* clear the screen */
	/*SDL_FillRect(s,NULL,black); */

	/* draw the wav from the saved stream buffer */
	for(x=0;x<W*2;x++)
	{
		const int X=x>>1, b=x&1 ,t=H4+H2*b;
		int y1,h1;
		if(buf[x]<0)
		{
			h1=-Y(buf[x]);
			y1=t-h1;
		}
		else
		{
			y1=t;
			h1=Y(buf[x]);
		}
		{
			SDL_Rect r={X,H2*b,1};
			r.h=y1-r.y;
			SDL_FillRect(s,&r,0);
		}
		{
			SDL_Rect r={X,y1,1,h1};
			SDL_FillRect(s,&r,white);
		}
		{
			SDL_Rect r={X,y1+h1,1};
			r.h=H2+H2*b-r.y;
			SDL_FillRect(s,&r,0);
		}
	}
	SDL_UnlockSurface(s);
	SDL_Flip(s);
	flips++;
}

/******************************************************************************/

void print_init_flags(int flags)
{
#define PFLAG(a) if(flags&MIX_INIT_##a) printf(#a " ")
	PFLAG(FLAC);
	PFLAG(MOD);
	PFLAG(MP3);
	PFLAG(OGG);
	if(!flags)
		printf("None");
	printf("\n");
}

/******************************************************************************/

int main(int argc, char **argv)
{
	int audio_rate,audio_channels;
	Uint16 audio_format;
	Uint32 t;
	Mix_Music *music;
	int volume=SDL_MIX_MAXVOLUME;

	/* initialize SDL for audio and video */
	if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO)<0)
		cleanExit("SDL_Init");
	atexit(SDL_Quit);

	int initted=Mix_Init(0);
	// printf("Before Mix_Init SDL_mixer supported: ");
	// print_init_flags(initted);
	initted=Mix_Init(~0);
	// printf("After  Mix_Init SDL_mixer supported: ");
	// print_init_flags(initted);
	Mix_Quit();

	/* open a screen for the wav output */
	if(!(s=SDL_SetVideoMode(W,H,(argc>2?atoi(argv[2]):8),(argc>3?SDL_FULLSCREEN:0)|SDL_HWSURFACE|SDL_DOUBLEBUF)))
		cleanExit("SDL_SetVideoMode");
	SDL_WM_SetCaption("sdlwav - SDL_mixer demo","sdlwav");
	
	/* hide the annoying mouse pointer */
	SDL_ShowCursor(SDL_DISABLE);
	/* get the colors we use */
	white=SDL_MapRGB(s->format,0xff,0xff,0xff);
	black=SDL_MapRGB(s->format,0,0,0);
	
	/* initialize sdl mixer, open up the audio device */
	if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,BUFFER)<0)
		cleanExit("Mix_OpenAudio");

	/* we play no samples, so deallocate the default 8 channels... */
	// Mix_AllocateChannels(0);
	
	/* print out some info on the audio device and stream */
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
	bits=audio_format&0xFF;
	sample_size=bits/8+audio_channels;
	rate=audio_rate;

	/* calculate some parameters for the wav display */
	dy=s->h/2.0/(float)(0x1<<bits);
	
	if(!(music=Mix_LoadMUS("loop1.wav")))
		cleanExit("Mix_LoadMUS(\"%s\")","loop1.wav");

	/* set the post mix processor up */
	Mix_SetPostMix(postmix,argv[1]);
	
	SDL_FillRect(s,NULL,black);
	SDL_Flip(s);
	SDL_FillRect(s,NULL,black);
	SDL_Flip(s);
	/* start playing and displaying the wav */
	/* wait for escape key of the quit event to finish */
	t=SDL_GetTicks();
	if(Mix_PlayMusic(music, 1)==-1)
		cleanExit("Mix_PlayMusic(0x%p,1)",music);
	Mix_VolumeMusic(volume);

	while((Mix_PlayingMusic() || Mix_PausedMusic()) && !done)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
				case SDL_QUIT:
					done=1;
					break;
				default:
					break;
			}
		}
		/* the postmix processor tells us when there's new data to draw */
		if(need_refresh)
			refresh();
		SDL_Delay(0);
	}
	t=SDL_GetTicks()-t;
	
	/* free & close */
	Mix_FreeMusic(music);
	Mix_CloseAudio();
	SDL_Quit();

	return(0);
}
