      SUBROUTINE UPSTR (STR, LEN)
C
C      UPSTR CONVERTS THE CHARACTER STRING STR TO UPPER CASE.
C      LEN IS THE NUMBER OF CHARACTERS TO CONVERT, NOT TO EXCEED THE
C      ACTUAL LENGTH OF STR.
C
C      AUTHOR: FRED KLEIN (U.S.G.S)
C
      CHARACTER            STR*(*)
      INTEGER                  I
      INTEGER                  J
      INTEGER                  LEN

      DO I = 1, LEN
        J = ICHAR(STR(I:I))
        IF (J .GT. 96 .AND. J .LT. 123) STR(I:I) =  CHAR(J - 32)
      END DO
      RETURN
      END

