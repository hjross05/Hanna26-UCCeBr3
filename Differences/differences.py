import ROOT as root
import sys

filename1= sys.argv[1]
filename2= sys.argv[2]

file1 = root.TFile.Open(filename1, 'read')

en1 = []

for i in range (9):
    en1.append(file1.Get(f'en{i}'))
    en1[-1].SetDirectory(0)

enAll1 = file1.Get('enAll')
enAll1.SetDirectory(0)

file1.Close()

#enAll1.Draw()

file2 = root.TFile.Open(filename2, 'read')

en2 = []

for i in range (9):
    en2.append(file2.Get(f'en{i}'))
    en2[-1].SetDirectory(0)

enAll2 = file2.Get('enAll')
enAll2.SetDirectory(0)

file2.Close()

difflist = []


for i in range (9):
    difflist.append(en1[i].Clone(f'diff{i}'))
    difflist[-1].Add(en2[i], -1)


diffAll = enAll1.Clone('diffAll')
diffAll.Add(enAll2, -1)

output_file = root.TFile.Open('TITLE!!.root', 'recreate')

for i in range (9):
    difflist[i].Write()

diffAll.Write()

output_file.Close()
print(f'Saved differences to {output_file}')

diffAll.Rebin(50)
diffAll.Draw()
