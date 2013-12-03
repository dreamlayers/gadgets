#include <stdio.h>
#include "cgic.h"
#include <string.h>
#include <stdlib.h>
#include "libsigntcp.h"

#define MSG_MAX 4096

static int ml;
static char *message;

static int MsgText() {
  if (cgiFormStringSpaceNeeded("M", &ml) != cgiFormSuccess) {
    return -1;
  }

  if (ml > MSG_MAX) {
    ml = MSG_MAX;
  }
  message = malloc(ml);
  if (message == NULL) {
    return -1;
  }
  message[0] = 0;

  cgiFormString("M", message, ml);

  return 0;
}

static char *chargens[] = {
  "A",
  "F",
  "S",
  "G",
  "a",
  "f",
  "s",
  "g"
};

#define CHARGEN_AUTO 0
#define CHARGEN_FIRM 1
#define CHARGEN_SOFT 2
#define CHARGEN_GDI 3

static int chargen;

static void CharGen() {
  cgiFormRadio("C", chargens, 8, &chargen, CHARGEN_AUTO);
  if (chargen > 3)
    chargen -= 4;
}

static int flags;
static int oneline;

static void CheckBoxes() {
  if (cgiFormCheckboxSingle("L") == cgiFormSuccess) {
    flags |= SFLAG_LOOP;
  }
  if (cgiFormCheckboxSingle("A") == cgiFormSuccess) {
    flags |= SFLAG_APPEND;
  }
  if (cgiFormCheckboxSingle("O") == cgiFormSuccess) {
    oneline = 1;
  } else {
    oneline = 0;
  }
  if (cgiFormCheckboxSingle("Q") == cgiFormSuccess) {
    flags |= SFLAG_QUEUE;
  }
}

static int transtm = 0;
static int gntm = 0;

static int HandleSubmit()
{
  char *p;

  flags = 0;

  CheckBoxes();
  CharGen();

  cgiFormIntegerBounded("G", &gntm, 0, 10, 0);
  if (gntm > 0) {
    gntm *= 100;
    flags |= SFLAG_GNTEE;
  }
  cgiFormIntegerBounded("T", &transtm, 0, 1000, 0);
  if (transtm > 0) {
    transtm *= 100;
    flags |= SFLAG_TRANS;
  }


  if (MsgText() != 0) return -1;

  /* Entries(); */

  /* If one line convert newlines to spaces */
  if (oneline) {
    p = message;
    while (*p != 0) {
      if (*p == '\n') *p = ' ';
      p++;
    }
  }

  /* Do auto character generator determination
   * Algorithm - use firmware unless:
   *  - mixed case
   *  - newlines
   *  - characters that cannot be displayed
   */
  if (chargen == CHARGEN_AUTO) {
    p = message;

    while (1) {
      if (*p == 0) {
        chargen = CHARGEN_FIRM;
        break;
      } else if (*p < 32 || *p > 126) {
        chargen = CHARGEN_SOFT;
        break;
      } else if (*p >= 'A' && *p <= 'Z') {
    if (chargen == CHARGEN_SOFT) {
          break;
        } else {
          chargen = CHARGEN_FIRM;
        }
      } else if (*p >= 'a' && *p <= 'z') {
    if (chargen == CHARGEN_FIRM) {
          chargen = CHARGEN_SOFT;
          break;
        } else {
          chargen = CHARGEN_SOFT;
        }
      }
      p++;
    }
  }

  if (chargen == CHARGEN_FIRM) {
    sc_hprint(message, flags, transtm, gntm);
#ifdef WIN32
  } else if (chargen == CHARGEN_GDI) {
    sc_gprint(message, flags, transtm, gntm);
#endif
  } else {
    sc_sprint(message, flags, transtm, gntm);
  }

  return 0;
}

static void ShowForm()
{
fprintf(cgiOut,
"<P ALIGN=\"CENTER\">\n"
#ifdef WIN32
"<FORM ACTION=\"/cgi-bin/lsp.exe\" METHOD=\"POST\">\n"
#else
"<FORM ACTION=\"/cgi-bin/lsp\" METHOD=\"POST\">\n"
#endif
"\n"
"<LABEL ACCESSKEY=M FOR=msgbuf><U>M</U>essage:</LABEL><BR>\n"
"<TEXTAREA name=\"M\" rows=12 cols=40 wrap=\"virtual\" ID=msgbuf>\n"
"</TEXTAREA><BR>\n"
"\n"
"<TABLE>\n"
"<TR><TD>\n"
"\n"
"<FIELDSET>\n"
"<LEGEND>Options</LEGEND>\n"
"<TABLE>\n"
"<TR><TD><INPUT TYPE=\"checkbox\" NAME=\"L\" ID=prtloop></TD>\n"
"<TD ALIGN=left><LABEL ACCESSKEY=L FOR=prtloop><U>L</U>oop</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=\"checkbox\" NAME=\"A\" ID=prtapnd></TD>\n"
"<TD ALIGN=left><LABEL ACCESSKEY=A FOR=prtapnd><U>A</U>ppend</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=\"checkbox\" NAME=\"O\" ID=prtoln></TD>\n"
"<TD ALIGN=left><LABEL ACCESSKEY=O FOR=prtoln><U>O</U>ne line</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=\"checkbox\" NAME=\"Q\" ID=qonly></TD>\n"
"<TD ALIGN=left><LABEL ACCESSKEY=Q FOR=qonly><U>Q</U>ueue only</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=text SIZE=3 MAXLENGTH=6 NAME=\"T\" VALUE=\"0\" ID=trans> s</TD>\n"
"<TD ALIGN=left><LABEL FOR=trans ACCESSKEY=T><U>T</U>ransience</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=text SIZE=3 MAXLENGTH=4 NAME=\"G\" VALUE=\"0\" ID=guar> s</TD>\n"
"<TD ALIGN=left><LABEL FOR=guar ACCESSKEY=G><U>G</U>uarantee</LABEL></TD></TR>\n"
"</TABLE>\n"
"</FIELDSET>\n"
"\n"
"</TD><TD>\n"
"\n"
"<FIELDSET>\n"
"<LEGEND>Character generator</LEGEND>\n"
"<TABLE>\n"
"<TR><TD><INPUT TYPE=radio NAME=\"C\" VALUE=\"A\" CHECKED ID=charap></TD>\n"
"<TD ALIGN=left><LABEL FOR=charap ACCESSKEY=U>A<U>u</U>to-pick</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=radio NAME=\"C\" VALUE=\"F\" ID=charfirm></TD>\n"
"<TD ALIGN=left><LABEL FOR=charfirm ACCESSKEY=F><U>F</U>irmware</LABEL></TD></TR>\n"
"<TR><TD><INPUT TYPE=radio NAME=\"C\" VALUE=\"S\" ID=charsoft></TD>\n"
"<TD ALIGN=left><LABEL FOR=charsoft ACCESSKEY=S><U>S</U>oftware</LABEL></TD></TR>\n"
#ifdef WIN32
"<TR><TD><INPUT TYPE=radio NAME=\"C\" VALUE=\"G\" ID=chargdi></TD>\n"
"<TD ALIGN=left><LABEL FOR=chargdi ACCESSKEY=G><U>G</U>DI</LABEL></TD></TR>\n"
#endif
"</TABLE>\n"
"</FIELDSET>\n"
"\n"
"</TD></TR>\n"
"\n"
"<TR><TD COLSPAN=2>\n"
"<INPUT TYPE=\"SUBMIT\" VALUE=\"Submit\">\n"
"<INPUT TYPE=\"RESET\" VALUE=\"Reset\" NAME=\"reset\">\n"
"</TD></TR>\n"
"\n"
"</TABLE>\n"
"\n"
"</FORM>\n");
}

/* cgic.h from cgic205 doesn't supply a full prototype for cgiMain() */
int cgiMain(void);
int cgiMain(void) {
  /* Send the content type, letting the browser know this is HTML */
  cgiHeaderContentType("text/html");

  /* Top of the page */
  fprintf(cgiOut,
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"\n"
"        \"http://www.w3.org/TR/REC-html40/loose.dtd\">\n"
"<HTML>\n"
"<HEAD>\n"
"<TITLE>\n"
"LED Sign Print\n"
"</TITLE>\n"
"</HEAD>\n"
"<BODY>\n"
"<H1 ALIGN=\"CENTER\">LED Sign Print</H1>\n");

  /* If a submit button has already been clicked, act on the
     submission of the form. */
  if (1 || cgiFormSubmitClicked("Submit") == cgiFormSuccess) {
    HandleSubmit();
    fprintf(cgiOut, "<hr>\n");
  }

  /* Now show the form */
  ShowForm();

  /* Finish up the page */
  fprintf(cgiOut, "</BODY></HTML>\n");
  return 0;
}
