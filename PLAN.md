## Aggiornamento del timeout
L'aggiornamento del timeout segue una regola del tipo Multiplicative Increase, Averaging Decrease

Sia $t(k)$ il timeout impostato all'unità di tempo $k$ e $T$ il timeout "ideale" per l'interazione client-server

La legge di controllo per $t(k)$ la scelgo in modo che
$$
    t(k+1) = 
        \begin{cases}
            q\cdot t(k)                     & t(k) < T \\
            \alpha\cdot t(k) + \beta\cdot T & t(k) \geq T
        \end{cases}
$$
dove $q \geq 1$ mentre $\alpha + \beta = 1$ e $0 < \alpha < 1$

Nel caso più generico in cui il timer ideale varii nel tempo dovremmo sostituire $T(k)$ a $T$

Nel caso di $T(k)$ costante possiamo osservare che, per timeout iniziale $\tau = t(0)$ sufficientemente piccolo, l'evoluzione nel tempo di $t$ sarà dato dalla seguente lista di valori
$$
    \tau \mapsto q\tau \mapsto q^2\tau \mapsto \cdots q^n\tau > T
$$
giunto oltre il valore di $T$ la legge di controllo imporrà di fare una media pesata tra $t(k)$ e $T$, quindi
$$
    q^n\tau
    \mapsto
    \alpha q^n\tau + \beta T
    \mapsto
    \alpha^2 q^n\tau + \alpha\beta T + \beta T
    \mapsto
    \cdots
    \mapsto
    \alpha^m q^n\tau + \sum_{i = 0}^{m - 1} \alpha^i\beta T
$$

A regime il valore di $t(k)$ sarà dato da
$$
    \lim_{m\to\infty}
    \alpha^m q^n\tau + \sum_{i = 0}^{m - 1} \alpha^i\beta T
$$

Ora $\alpha^m\to+0$ in quanto $|\alpha| < 1$ mentre la sommatoria altro non è che una serie geometrica con rapporto comune pari ad $\alpha$ (e siccome $|\alpha| < 1$ sarà pertanto una serie convergente)
$$
    t(k\to+\infty) = \frac 1{1 - \alpha}\beta T
$$
e siccome $\alpha + \beta = 1$, $1 - \alpha = \beta$ quindi
$$
    t(k\to+\infty) = T
$$
