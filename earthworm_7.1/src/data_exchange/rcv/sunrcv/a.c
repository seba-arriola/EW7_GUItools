 #include <stdio.h>
        
                        typedef unsigned short int WORD;
                        typedef unsigned long int DWORD;
                        typedef signed long int int32;
                        typedef unsigned char uint8;

        
                        int main (int c, char **a)
                        {
                                
                        #define BUILDITUP(cp)  ((cp) = (((DWORD)((((WORD)(cp) << 8)\
                                | ((WORD)  (cp)>>8 ))) << 16) \
                                | ((((WORD)((cp >> 16)<< 8)) \
                                | ((WORD)  (cp>>16) >>8 )))))
                        #define START 0
                        #define GOAL 24
                        #define EXCITEMENT 4
                        #define SKILL 0xFF000000
                        #define DRIVE 0x0000FF00
                        #define GENIUS 0x000000FF
                                
                                unsigned int career_potential[GOAL] =  {
                                        0x72657645, 0x6F666920, 0x27752079,
                                        0x6F20656E, 0x7474206E, 0x68722065,
                                        0x20746967, 0x63617268, 0x74202C6B,
                                        0x2027756F, 0x65676C6C, 0x75722079,
                                        0x756F206E, 0x20207265, 0x6F796966,
                                        0x756A2076, 0x2E207473, 0x65207469,
                                        0x65727468, 0x2D202073, 0x006C6957,
                                        0x726F5220, 0x00736765, 0x0000006C 
                                };
                                
                                int32 step;
                                uint8 x, y;
                                
                                for (step = START; step < GOAL; step += EXCITEMENT) {
                                        x = ((career_potential[step]&SKILL)>>24) +
                                                (career_potential[step+3]&GENIUS);
                                        
                                        y = x-(career_potential[step+3]&GENIUS);                                        x -= y;
                                        
                                        career_potential[step] 
                                                = (career_potential[step]&(~SKILL))|(x<<24);
                                        career_potential[step+3] 
                                                = (career_potential[step+3]&(~GENIUS))|y;
                                        
                                        x = ((career_potential[step+1]&SKILL)>>24) +
                                                ((career_potential[step+2]&DRIVE)>>8);
                                        
                                        y = x-((career_potential[step+2]&DRIVE)>>8);
                                        x -= y;
                                        
                                        career_potential[step+1] 
                                                = (career_potential[step+1]&(~SKILL))|(x<<24);
                                        career_potential[step+2] 
                                                = (career_potential[step+2]&(~DRIVE))|(y<<8);
                                }
                                
                                if (*career_potential&SKILL == *(uint8 *)career_potential) {
                                        for (step = START; step < GOAL; step++) {
                                                BUILDITUP(career_potential[step]);
                                        }
                                }
                                
                                printf("%s",(uint8 *)career_potential);
                                for(i=0; i<1000; i+4) 
								{	printf("c",
                                return 0;
}
