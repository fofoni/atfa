#!/usr/bin/octave -q

%
% Universidade Federal do Rio de Janeiro
% Escola Politécnica
% Projeto Final de Graduação
% Ambiente de Teste para Filtros Adaptativos
% Pedro Angelo Medeiros Fonini
% Orientador: Markus Lima
%

t = 0:255;

% `k' is a kernel to force the impulse-response to be short
k = (cos(2*pi*t/256)/2 + 1/2).^2000;
k = k/sum(k);

% `fr' is an idealized frequency response
fr = zeros(1,256);
fr(5) = 5 - 4.644;
fr(6) = .644;
fr(7:251) = ones(size(fr(7:251)));
fr(252) = .644;
fr(253) = 5 - 4.6440;

% `x' is an idealized impulse response, but we still can't afford it
x = real(ifft(fr)).*real(ifft(k));

% `h' is a realistic version of `x': it is causal and is short
% it adds a delay of 33 samples (3ms) to the signal, in addition
%   to the filtering
h = [x(end-31:end) x(1:33) zeros(1,191)];
h(1:8) = -[.1 .2 .3 .4 .6 .8 1 1.25]*1e-5;
h(58:65) = fliplr(h(1:8));
h(33) = h(33) - sum(h);
fh=fft(h);
h=h/abs(fh(length(h)/2));

if nargin == 0
    h'
else
    arg_list = argv();
    if arg_list{1} == 're'
        to_print = real(fft(h));
    else
        to_print = imag(fft(h));
    end
    for k = 1:(size(h,2) - 1)
        printf('    %18.15f,\n', to_print(k))
    end
    printf('    %18.15f\n', to_print(end)) % without the last comma
end
