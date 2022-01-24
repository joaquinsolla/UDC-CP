%%JOAQUÍN SOLLA VÁZQUEZ - joaquin.solla@udc.es
%%COMPAÑERO: SAÚL LEYVA SANTARÉN - saul.leyva.santaren@udc.es
%%GROUP 2.5

-module(break_md5).
-define(PASS_LEN, 6).
-define(UPDATE_BAR_GAP, 1000).
-define(BAR_SIZE, 40).
-define(NUM_PROC, 3). 

-export([break_md5s/1, break_md5/1, pass_to_num/1, num_to_pass/1, update/3]).
-export([progress_loop/2, send_message/2, end_message/1, empty/1]).
-export([break_md5/4]).
-export([loop_start/7]).

% Base ^ Exp

pow_aux(_Base, Pow, 0) -> Pow;
pow_aux(Base, Pow, Exp) when Exp rem 2 == 0 ->
    pow_aux(Base*Base, Pow, Exp div 2);
pow_aux(Base, Pow, Exp) -> pow_aux(Base, Base * Pow, Exp - 1).

pow(Base, Exp) -> pow_aux(Base, 1, Exp).

%% Number to password and back conversion

num_to_pass_aux(_N, 0, Pass) -> Pass;
num_to_pass_aux(N, Digit, Pass) ->
    num_to_pass_aux(N div 26, Digit - 1, [$a + N rem 26 | Pass]).

num_to_pass(N) -> num_to_pass_aux(N, ?PASS_LEN, []).

pass_to_num_aux([], Num) -> Num;
pass_to_num_aux([C|T], Num) -> pass_to_num_aux(T, Num*26 + C-$a).

pass_to_num(Pass) -> pass_to_num_aux(Pass, 0).

%% Hex string to Number

hex_char_to_int(N) ->
    if (N >= $0) and (N =< $9) -> N-$0;
       (N >= $a) and (N =< $f) -> N-$a+10;
       (N >= $A) and (N =< $F) -> N-$A+10;
       true -> throw({not_hex, [N]})
    end.

hex_string_to_num_aux([], Num) -> Num;
hex_string_to_num_aux([Hex|T], Num) ->
    hex_string_to_num_aux(T, Num*16 + hex_char_to_int(Hex)).

hex_string_to_num(Hex) -> hex_string_to_num_aux(Hex, 0).

%% Progress bar runs in its own process

progress_loop(N, Bound) ->
    receive
        stop -> ok;
        {progress_report, Checked} ->
            N2 = N + Checked,
            Full_N = N2 * ?BAR_SIZE div Bound,
            Full = lists:duplicate(Full_N, $=),
            Empty = lists:duplicate(?BAR_SIZE - Full_N, $-),
            io:format("\r[~s~s] ~.2f%", [Full, Empty, N2/Bound*100]),
            progress_loop(N2, Bound)
    end.

%% break_md5/2 iterates checking the possible passwords

break_md5([], _, _, _) -> receive {continue, From} -> From ! {finished, self()} end; % No more hashes to find
break_md5(_,  N, N, _) -> receive {continue, From} -> From ! {not_found, self()} end;  % Checked every possible password
break_md5(Hashes, N, Bound, Progress_Pid) ->
    receive 
        {continue, From} ->
            if N rem ?UPDATE_BAR_GAP == 0 ->
                    Progress_Pid ! {progress_report, ?UPDATE_BAR_GAP};
            true ->
                    ok
            end,
            Pass = num_to_pass(N),
            Hash = crypto:hash(md5, Pass),
            Num_Hash = binary:decode_unsigned(Hash),
            case lists:member(Num_Hash, Hashes) of
                true ->
                    io:format("\e[2K\r~.16B: ~s~n", [Num_Hash, Pass]),
                    From ! {new_hash, self(), Num_Hash},
                    break_md5(lists:delete(Num_Hash, Hashes), N+1, Bound, Progress_Pid);
                false -> 
                    self() ! {continue, From},
                    break_md5(Hashes, N+1, Bound, Progress_Pid)
            end;
        finish -> ok
    end.
    
    
%% Start all threads

loop_start(First, Last, Bound, Iter_Proc, Progress_Pid, Num_Hashes, List) ->

    if Last+Iter_Proc >= Bound ->
        Aux = spawn(?MODULE, break_md5, [Num_Hashes, First, Bound, Progress_Pid]),
        Auxlist = lists:append(List, [Aux]),
        update(Auxlist, Num_Hashes, Progress_Pid);
    true ->
        Aux = spawn(?MODULE, break_md5, [Num_Hashes, First, Last, Progress_Pid]),
        Auxlist = lists:append(List, [Aux]),
        loop_start(First+Iter_Proc, Last+Iter_Proc, Bound, Iter_Proc, Progress_Pid, Num_Hashes, Auxlist)
    end. 
    

%% Comunicates with each of the processes and terminates them   
    
update(Prosses_List, Pass_List, Progress_Pid) ->
    receive
        {finished, Proc}-> end_message(lists:delete(Proc, Prosses_List)), Progress_Pid ! stop, ok;
        {new_hash, Proc, H} -> case empty(lists:delete(H, Pass_List)) of
                                    true -> end_message(Prosses_List), Progress_Pid ! stop, ok;
                                    false -> Proc ! {continue, self()}, update(Prosses_List, lists:delete(H, Pass_List), Progress_Pid)
                                end;
        {not_found, Proc} -> case empty(lists:delete(Proc, Prosses_List)) of
                                    true -> end_message(lists:delete(Proc, Prosses_List)), Progress_Pid ! stop, not_found;%0 procesos
                                    false -> update(lists:delete(Proc, Prosses_List), Pass_List, Progress_Pid)
                             end
        after 10 -> send_message(Prosses_List, self()), update(Prosses_List, Pass_List, Progress_Pid) %inicializa por primera vez
    end.
 
 
%% Sends to Pid process a message to continue    
    
send_message([], _) -> ok;
send_message([H|T], Pid) ->
    H ! {continue, Pid},
    send_message(T, Pid).
    
  
%% Sends to Pid process a message to finish       
    
end_message([]) -> ok;
end_message([H|T]) ->
    H ! finish,
    end_message(T).


%% Checks if a list is empty or not

empty([]) -> true;
empty(_) -> false.


%% Break a list of hashes

break_md5s(Hashes) ->
    Bound = pow(26, ?PASS_LEN),
    Progress_Pid = spawn(?MODULE, progress_loop, [0, Bound]),
    Num_Hashes = lists:map(fun hex_string_to_num/1, Hashes),
    Iter_Proc = Bound div ?NUM_PROC, 
    loop_start(0, Iter_Proc, Bound, Iter_Proc, Progress_Pid, Num_Hashes, []),
    ok.

%% Break a single hash

break_md5(Hash) -> break_md5s([Hash]).
