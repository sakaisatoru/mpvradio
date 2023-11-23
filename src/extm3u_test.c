#include <stdio.h>
#include <errno.h>
#include <string.h>

#define FALSE   (0)
#define TRUE    (!FALSE)

int main (void)
{
	int f;
	char buf[256];
    char *header, *tail, *url;
    int i, flag = FALSE;

	FILE *fp = fopen ("/var/lib/mpd/playlists/00_radiko.m3u","r");
	//~ FILE *fp = fopen ("radio.m3u","r");
	if (fp != NULL) {
		while (!feof(fp)) {
		    if (fgets (buf, sizeof(buf)-1, fp) == NULL) {
		        if (ferror(fp)) {
	                if (errno) {
		                printf ("errno : %d  %s\n", errno, strerror(errno));
	                }
	                break;	
		        }
		    }
		    if (flag) {
		        if (*buf != '#') {
	                url = buf;
	                tail = strrchr (url, '\n');
	                if (tail != NULL) *tail = '\0';
	                printf ("  url : %s\n", url);
	                flag = FALSE;
	            }
	        }
	        if (sscanf (buf, "#EXTINF:%d,%*[A-Za-z0-9()_. ] / %*s\n", &i)) {
	            header = strrchr (buf, '/');
	            if (header != NULL) {
	                header++;
	                tail = strrchr (header, '\n');
	                if (tail != NULL) *tail = '\0';
	            }
	            else {
	                header = "";
	            }
	            flag = TRUE;
	            printf ("length : %d  label : %s\t\t", i, header);
	        }
	    }
	}
	else {
	    printf ("file not found.\n");
	}
	
	return 0;
}

